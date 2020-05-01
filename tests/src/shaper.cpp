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
	void checkShaper(float offsetX)
	{
		Shaper shaper{ kFirstY, kDeltaY, kDeltaX, offsetX };
		for (float i = offsetX; i < kDeltaX; ++i)
			CHECK(shaper.advance() == doctest::Approx{ Shaper::value(kFirstY, kDeltaY, kDeltaX, i) }.epsilon(kPrecision));
	}
}

TEST_CASE("shaper_cosine_full")
{
	::checkShaper<aulos::CosineShaper>(0);
}

TEST_CASE("shaper_cosine_half")
{
	::checkShaper<aulos::CosineShaper>(kDeltaX / 2);
}

TEST_CASE("shaper_cubic_full")
{
	::checkShaper<aulos::CubicShaper>(0);
}

TEST_CASE("shaper_cubic_half")
{
	::checkShaper<aulos::CubicShaper>(kDeltaX / 2);
}

TEST_CASE("shaper_linear_full")
{
	::checkShaper<aulos::LinearShaper>(0);
}

TEST_CASE("shaper_linear_half")
{
	::checkShaper<aulos::LinearShaper>(kDeltaX / 2);
}

TEST_CASE("shaper_quadratic_full")
{
	::checkShaper<aulos::QuadraticShaper>(0);
}

TEST_CASE("shaper_quadratic_half")
{
	::checkShaper<aulos::QuadraticShaper>(kDeltaX / 2);
}
