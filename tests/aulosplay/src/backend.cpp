// This file is part of the Aulos toolkit.
// Copyright (C) Sergei Blagodarin.
// SPDX-License-Identifier: Apache-2.0

#include <aulosplay/src/backend.hpp>

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
				CHECK(_framesRemaining == 0);
				CHECK(_stopFlag);
			}
		}

		const std::atomic<bool>& stopFlag() const
		{
			return _stopFlag;
		}

	private:
		size_t onDataExpected(float* output, size_t maxFrames, float*) noexcept override
		{
			CHECK(maxFrames > 0);
			const auto result = std::min(_framesRemaining, maxFrames);
			if (result > 0)
			{
				std::memset(output, 0, result * aulosplay::kBackendFrameBytes);
				_framesRemaining -= result;
			}
			else
			{
				CHECK(!_stopFlag);
				_stopFlag = true;
			}
			MESSAGE(++_step, ") ", maxFrames, " -> ", result);
			return result;
		}

		void onDataProcessed() override
		{
		}

		void onErrorReported(aulosplay::PlaybackError error) override
		{
			REQUIRE(error == aulosplay::PlaybackError::NoDevice);
			CHECK(_step == 0);
			CHECK(_framesRemaining == kTestFrames);
			CHECK(!_stopFlag);
			MESSAGE("No audio playback device found");
			_skipPostconditions = true;
		}

		void onErrorReported(const char* function, int code, const std::string& description) override
		{
			FAIL_CHECK(description, " (", function, " -> ", code, ")");
			_skipPostconditions = true;
		}

	private:
		std::atomic<bool> _stopFlag{ false };
		unsigned _step = 0;
		size_t _framesRemaining = kTestFrames;
		bool _skipPostconditions = false;
	};
}

TEST_CASE("backend")
{
	BackendTester tester;
	aulosplay::runBackend(tester, kTestSamplingRate, tester.stopFlag());
	tester.checkPostconditions();
}
