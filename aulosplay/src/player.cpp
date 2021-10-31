// This file is part of the Aulos toolkit.
// Copyright (C) Sergei Blagodarin.
// SPDX-License-Identifier: Apache-2.0

#include <aulosplay/player.hpp>

#include "backend.hpp"

#include <primal/buffer.hpp>

#include <atomic>
#include <cstdio>
#include <mutex>
#include <thread>

namespace
{
	class PlayerImpl final
		: public aulosplay::Player
		, private aulosplay::BackendCallbacks
	{
	public:
		PlayerImpl(aulosplay::PlayerCallbacks& callbacks, unsigned samplingRate)
			: _callbacks{ callbacks }
			, _samplingRate{ samplingRate }
			, _thread{ [this] { runBackend(*this, _samplingRate); } }
		{
		}

		~PlayerImpl() noexcept override
		{
			_done.store(true);
			_thread.join();
		}

		void play(const std::shared_ptr<aulosplay::Source>& source) override
		{
			auto inOut = source;
			std::lock_guard lock{ _mutex };
			_source.swap(inOut);
		}

		unsigned samplingRate() const noexcept override
		{
			return _samplingRate;
		}

		void stop() noexcept override
		{
			decltype(_source) out;
			std::lock_guard lock{ _mutex };
			_source.swap(out);
		}

	private:
		void onBackendAvailable(size_t maxReadFrames) override
		{
			_monoBuffer.reserve(maxReadFrames, false);
		}

		void onBackendError(aulosplay::PlaybackError error) override
		{
			_callbacks.onPlaybackError(error);
		}

		void onBackendError(const char* function, int code, const std::string& description) override
		{
			std::string message;
			if (description.empty())
			{
				constexpr auto pattern = "[%s] Error 0x%08X";
				message.resize(static_cast<size_t>(std::snprintf(nullptr, 0, pattern, function, code)), '\0');
				std::snprintf(message.data(), message.size() + 1, pattern, function, code);
			}
			else
			{
				constexpr auto pattern = "[%s] Error 0x%08X: %s";
				message.resize(static_cast<size_t>(std::snprintf(nullptr, 0, pattern, function, code, description.c_str())), '\0');
				std::snprintf(message.data(), message.size() + 1, pattern, function, code, description.c_str());
			}
			_callbacks.onPlaybackError(std::move(message));
		}

		bool onBackendIdle() override
		{
			if (_started)
				_callbacks.onPlaybackStarted();
			if (_stopped)
				_callbacks.onPlaybackStopped();
			return !_done.load();
		}

		size_t onBackendRead(float* output, size_t maxFrames) noexcept override
		{
			size_t frames = 0;
			bool monoToStereo = false;
			if (std::lock_guard lock{ _mutex }; _source)
			{
				if (_source->isStereo())
					frames = _source->onRead(output, maxFrames);
				else
				{
					frames = _source->onRead(_monoBuffer.data(), maxFrames);
					monoToStereo = true;
				}
				if (frames < maxFrames)
					_source.reset();
			}
			if (monoToStereo)
				primal::duplicate1D_32(output, _monoBuffer.data(), frames);
			_started = !_playing && frames > 0;
			_stopped = _playing && frames < maxFrames;
			_playing = frames == maxFrames;
			return frames;
		}

	private:
		aulosplay::PlayerCallbacks& _callbacks;
		const unsigned _samplingRate;
		primal::Buffer<float, primal::AlignedAllocator<primal::kDspAlignment>> _monoBuffer;
		std::atomic<bool> _done{ false };
		std::shared_ptr<aulosplay::Source> _source;
		bool _playing = false;
		bool _started = false;
		bool _stopped = false;
		std::mutex _mutex;
		std::thread _thread;
	};
}

namespace aulosplay
{
	std::unique_ptr<Player> Player::create(PlayerCallbacks& callbacks, unsigned samplingRate)
	{
		return std::make_unique<PlayerImpl>(callbacks, samplingRate);
	}
}
