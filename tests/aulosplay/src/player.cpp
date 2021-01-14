// This file is part of the Aulos toolkit.
// Copyright (C) Sergei Blagodarin.
// SPDX-License-Identifier: Apache-2.0

#include <aulosplay/player.hpp>

#include <cstring>
#include <thread>

#include <ostream>
#include <doctest.h>

namespace
{
	constexpr size_t kTestFrames = 32'783; // A prime number.

	struct TestCallbacks
		: public aulosplay::Source
		, public aulosplay::PlayerCallbacks
	{
		const size_t _channels;
		bool _started = false;
		unsigned _step = 0;
		size_t _framesRemaining = kTestFrames;
		bool _skipPostconditions = false;

		TestCallbacks(bool stereo) noexcept
			: _channels{ stereo ? 2u : 1u } {}

		bool isStereo() const noexcept override
		{
			return _channels == 2;
		}

		size_t onRead(float* buffer, size_t maxFrames) noexcept override
		{
			CHECK(maxFrames > 0);
			const auto result = std::min(_framesRemaining, maxFrames);
			if (result > 0)
			{
				std::memset(buffer, 0, result * _channels * sizeof(float));
				_framesRemaining -= result;
			}
			MESSAGE(++_step, ") ", maxFrames, " -> ", result);
			return result;
		}

		void onPlaybackError(aulosplay::PlaybackError error) override
		{
			REQUIRE(error == aulosplay::PlaybackError::NoDevice);
			CHECK(_step == 0);
			CHECK(_framesRemaining == kTestFrames);
			MESSAGE("No audio playback device found");
			_skipPostconditions = true;
		}

		void onPlaybackError(std::string&& message) override
		{
			FAIL_CHECK(message);
			_skipPostconditions = true;
		}

		void onPlaybackStarted() override
		{
			CHECK(!_started);
			_started = true;
		}

		void onPlaybackStopped() override
		{
			CHECK(_started);
			_started = false;
		}
	};
}

TEST_CASE("player_mono")
{
	const auto callbacks = std::make_shared<TestCallbacks>(false);
	{
		const auto player = aulosplay::Player::create(*callbacks, 48'000);
		REQUIRE(player);
		player->play(callbacks);
		std::this_thread::sleep_for(std::chrono::seconds{ 1 });
	}
	if (!callbacks->_skipPostconditions)
	{
		CHECK(!callbacks->_started);
		CHECK(callbacks->_framesRemaining == 0);
	}
}

TEST_CASE("player_stereo")
{
	const auto callbacks = std::make_shared<TestCallbacks>(true);
	{
		const auto player = aulosplay::Player::create(*callbacks, 48'000);
		REQUIRE(player);
		player->play(callbacks);
		std::this_thread::sleep_for(std::chrono::seconds{ 1 });
	}
	if (!callbacks->_skipPostconditions)
	{
		CHECK(!callbacks->_started);
		CHECK(callbacks->_framesRemaining == 0);
	}
}
