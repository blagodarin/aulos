// This file is part of the Aulos toolkit.
// Copyright (C) Sergei Blagodarin.
// SPDX-License-Identifier: Apache-2.0

#include <aulosplay/player.hpp>

#include "common.hpp"

#include <condition_variable>
#include <cstring>
#include <mutex>
#include <thread>

#include <ostream>
#include <doctest/doctest.h>

namespace
{
	class SingleSourcePlayerTester
		: public aulosplay::Source
		, public aulosplay::PlayerCallbacks
	{
	public:
		SingleSourcePlayerTester(size_t channels) noexcept
			: _channels{ channels } {}

		void checkPostconditions() const
		{
			std::lock_guard lock{ _mutex };
			if (!_skipPostconditions)
			{
				CHECK(_started);
				CHECK(_stopped);
				CHECK(_framesRemaining == 0);
			}
		}

		void waitForStop()
		{
			std::unique_lock lock{ _mutex };
			CHECK(_condition.wait_for(lock, std::chrono::seconds{ 5 }, [this] { return _stopped; }));
		}

	private:
		bool isStereo() const noexcept override
		{
			return _channels == 2;
		}

		size_t onRead(float* buffer, size_t maxFrames) noexcept override
		{
			CHECK(maxFrames > 0);
			std::lock_guard lock{ _mutex };
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
			MESSAGE("No audio playback device found");
			{
				std::lock_guard lock{ _mutex };
				CHECK(!_started);
				_stopped = true;
				_skipPostconditions = true;
			}
			_condition.notify_one();
		}

		void onPlaybackError(std::string&& message) override
		{
			FAIL_CHECK(message);
			{
				std::lock_guard lock{ _mutex };
				CHECK(!_stopped);
				_stopped = true;
				_skipPostconditions = true;
			}
			_condition.notify_one();
		}

		void onPlaybackStarted() override
		{
			std::lock_guard lock{ _mutex };
			CHECK(!_started);
			_started = true;
		}

		void onPlaybackStopped() override
		{
			{
				std::lock_guard lock{ _mutex };
				CHECK(_started);
				CHECK(!_framesRemaining);
				CHECK(!_stopped);
				_stopped = true;
			}
			_condition.notify_one();
		}

	private:
		const size_t _channels;
		mutable std::mutex _mutex;
		std::condition_variable _condition;
		bool _started = false;
		unsigned _step = 0;
		size_t _framesRemaining = kTestFrames;
		bool _stopped = false;
		bool _skipPostconditions = false;
	};
}

TEST_CASE("player_single_source")
{
	std::shared_ptr<SingleSourcePlayerTester> tester;
	SUBCASE("mono")
	{
		tester = std::make_shared<SingleSourcePlayerTester>(1u);
	}
	SUBCASE("stereo")
	{
		tester = std::make_shared<SingleSourcePlayerTester>(2u);
	}
	{
		const auto player = aulosplay::Player::create(*tester, kTestSamplingRate);
		REQUIRE(player);
		player->play(tester);
		tester->waitForStop();
	}
	tester->checkPostconditions();
}
