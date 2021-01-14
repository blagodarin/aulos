// This file is part of the Aulos toolkit.
// Copyright (C) Sergei Blagodarin.
// SPDX-License-Identifier: Apache-2.0

#include <aulosplay/src/backend.hpp>

#include <cstring>

#include <ostream>
#include <doctest.h>

#ifdef _WIN32
#	define NOMINMAX
#	define WIN32_LEAN_AND_MEAN
#	pragma warning(push)
#	pragma warning(disable : 5039) // pointer or reference to potentially throwing function passed to 'extern "C"' function under -EHc. Undefined behavior may occur if this function throws an exception.
#	include <mmdeviceapi.h>
#	pragma warning(pop)
#endif

namespace
{
	constexpr size_t kTestFrames = 32'783; // A prime number.

	struct TestCallbacks : public aulosplay::BackendCallbacks
	{
		unsigned _step = 0;
		size_t _framesRemaining = kTestFrames;
		std::atomic<bool> _stopFlag{ false };

		size_t onDataExpected(float* output, size_t maxFrames, float*) noexcept override
		{
			CHECK(maxFrames > 0);
			const auto result = std::min(_framesRemaining, maxFrames);
			if (result > 0)
			{
				std::memset(output, 0, result * aulosplay::kFrameBytes);
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

		void onErrorReported(const char* function, unsigned code, const std::string& description) override
		{
#ifdef _WIN32
			if (std::string_view{ function } == "IMMDeviceEnumerator::GetDefaultAudioEndpoint" && code == static_cast<unsigned>(E_NOTFOUND))
			{
				MESSAGE(description, " (", function, " -> ", code, ")");
				CHECK(_framesRemaining == kTestFrames);
				CHECK(!_stopFlag);
				_framesRemaining = 0;
				_stopFlag = true;
				return;
			}
#endif
			FAIL_CHECK(description, " (", function, " -> ", code, ")");
		}
	};
}

TEST_CASE("backend")
{
	TestCallbacks callbacks;
	aulosplay::runBackend(callbacks, 48'000, callbacks._stopFlag);
	CHECK(callbacks._framesRemaining == 0);
	CHECK(callbacks._stopFlag);
}
