// This file is part of the Aulos toolkit.
// Copyright (C) Sergei Blagodarin.
// SPDX-License-Identifier: Apache-2.0

#include <aulosplay/player.hpp>

#include "backend.hpp"
#include "utils.hpp"

namespace aulosplay
{
	Player::Player(PlayerCallbacks& callbacks, unsigned samplingRate)
		: _callbacks{ callbacks }
		, _samplingRate{ samplingRate }
	{
		_thread = std::thread{ [this] {
			runBackend(_callbacks, _samplingRate, _offset, _stop, [this](float* buffer, size_t frames, float* monoBuffer) {
				size_t result = 0;
				std::lock_guard lock{ _mutex };
				if (_source)
				{
					if (_source->isStereo())
						result = _source->onRead(buffer, frames);
					else
					{
						result = _source->onRead(monoBuffer, frames);
						monoToStereo(buffer, monoBuffer, result);
					}
					if (result < frames)
						_source.reset();
				}
				return result;
			});
		} };
	}

	Player::~Player()
	{
		_stop.store(true);
		_thread.join();
	}

	void Player::play(const std::shared_ptr<Source>& source)
	{
		auto inOut = source;
		std::lock_guard lock{ _mutex };
		_source.swap(inOut);
		_sourceChanged = true;
	}

	void Player::stop()
	{
		decltype(_source) out;
		std::lock_guard lock{ _mutex };
		_source.swap(out);
		_sourceChanged = true;
	}
}
