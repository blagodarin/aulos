//
// This file is part of the Aulos toolkit.
// Copyright (C) 2020 Sergei Blagodarin.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//

#include <aulos/src/shaper.hpp>

#include <doctest.h>

namespace
{
	constexpr float kFirstY = 1;
	constexpr float kDeltaY = -2;            // Y spans from 1 to -1.
	constexpr float kDeltaX = 48'000;        // Enough for 1 Hz sawtooth wave at 48 kHz sampling rate.
	constexpr double kPrecision = 0.00'0001; // Allow 0.0001% error.

	template <typename Shaper>
	void checkShaper()
	{
		Shaper shaper{ kFirstY, kDeltaY, kDeltaX, 0 };
		for (float i = 0; i < kDeltaX; ++i)
		{
			const auto nextValue = shaper.advance();
			CHECK(nextValue == doctest::Approx{ Shaper::value(kFirstY, kDeltaY, kDeltaX, i) }.epsilon(kPrecision));
			CHECK(nextValue == doctest::Approx{ Shaper{ kFirstY, kDeltaY, kDeltaX, i }.advance() }.epsilon(kPrecision));
		}
	}
}

TEST_CASE("shaper_cosine")
{
	::checkShaper<aulos::CosineShaper>();
}

TEST_CASE("shaper_cubic")
{
	::checkShaper<aulos::CubicShaper>();
}

TEST_CASE("shaper_linear")
{
	::checkShaper<aulos::LinearShaper>();
}

TEST_CASE("shaper_quadratic")
{
	::checkShaper<aulos::QuadraticShaper>();
}
