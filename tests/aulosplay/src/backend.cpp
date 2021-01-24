// This file is part of the Aulos toolkit.
// Copyright (C) Sergei Blagodarin.
// SPDX-License-Identifier: Apache-2.0

#include <aulosplay/src/backend.hpp>

#include <aulosplay/player.hpp>
#include "common.hpp"

#include <cstring>

#include <ostream>
#include <doctest.h>

namespace
{
	class BackendTester : public aulosplay::BackendCallbacks
	{
	public:
		void checkPostconditions() const
		{
			if (!_skipPostconditions)
			{
				CHECK(_available);
				CHECK(_stopping);
				CHECK(_framesRemaining == 0);
			}
		}

	private:
		void onBackendAvailable(size_t) override
		{
			CHECK(!_available);
			_available = true;
		}

		void onBackendError(aulosplay::PlaybackError error) override
		{
			CHECK(!_available);
			CHECK(!_stopping);
			REQUIRE(error == aulosplay::PlaybackError::NoDevice);
			CHECK(_step == 0);
			CHECK(_framesRemaining == kTestFrames);
			MESSAGE("No audio playback device found");
			_skipPostconditions = true;
		}

		void onBackendError(const char* function, int code, const std::string& description) override
		{
			CHECK(!_stopping);
			FAIL_CHECK(description, " (", function, " -> ", code, ")");
			_skipPostconditions = true;
		}

		bool onBackendIdle() override
		{
			CHECK(_available);
			if (!_shouldStop)
				return true;
			CHECK(!_stopping);
			_stopping = true;
			return false;
		}

		size_t onBackendRead(float* output, size_t maxFrames) noexcept override
		{
			CHECK(_available);
			CHECK(!_stopping);
			CHECK(maxFrames > 0);
			const auto result = std::min(_framesRemaining, maxFrames);
			if (result > 0)
			{
				std::memset(output, 0, result * aulosplay::kBackendFrameBytes);
				_framesRemaining -= result;
			}
			else
				_shouldStop = true;
			MESSAGE(++_step, ") ", maxFrames, " -> ", result);
			return result;
		}

	private:
		bool _available = false;
		bool _shouldStop = false;
		bool _stopping = false;
		unsigned _step = 0;
		size_t _framesRemaining = kTestFrames;
		bool _skipPostconditions = false;
	};
}

TEST_CASE("backend")
{
	BackendTester tester;
	aulosplay::runBackend(tester, kTestSamplingRate);
	tester.checkPostconditions();
}
