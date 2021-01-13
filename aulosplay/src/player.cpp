// This file is part of the Aulos toolkit.
// Copyright (C) Sergei Blagodarin.
// SPDX-License-Identifier: Apache-2.0

#include <aulosplay/player.hpp>

#include "backend.hpp"
#include "utils.hpp"

#include <atomic>
#include <cassert>
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
			, _thread{ [this] { runBackend(*this, _samplingRate, _stop); } }
		{
		}

		~PlayerImpl() noexcept override
		{
			_stop.store(true);
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
		size_t onDataExpected(float* output, size_t maxFrames, float* monoBuffer) noexcept override
		{
			size_t frames = 0;
			bool monoToStereo = false;
			if (std::lock_guard lock{ _mutex }; _source)
			{
				if (_source->isStereo())
					frames = _source->onRead(output, maxFrames);
				else
				{
					frames = _source->onRead(monoBuffer, maxFrames);
					monoToStereo = true;
				}
				if (frames < maxFrames)
					_source.reset();
			}
			if (monoToStereo)
				aulosplay::monoToStereo(output, monoBuffer, frames);
			_started = !_playing && frames > 0;
			_stopped = _playing && frames < maxFrames;
			_playing = frames == maxFrames;
			return frames;
		}

		void onDataProcessed() override
		{
			if (_started)
				_callbacks.onPlaybackStarted();
			if (_stopped)
				_callbacks.onPlaybackStopped();
		}

		void onErrorReported(const char* function, unsigned code, const std::string& description) override
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

	private:
		aulosplay::PlayerCallbacks& _callbacks;
		const unsigned _samplingRate;
		std::atomic<bool> _stop{ false };
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
