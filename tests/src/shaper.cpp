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

#include <aulos/src/shaper.hpp> // Included first to check header consistency.

#include <aulos/src/note_table.hpp>

#include <doctest.h>

namespace
{
	constexpr double kPrecision = 0.00'0001; // Allow 0.0001% error.

	template <typename Shaper>
	void checkShaper(float shapeParameter, double epsilon)
	{
		constexpr float kAmplitude = 1;
		constexpr float kDeltaY = -2 * kAmplitude;
		const auto kDeltaX = 48'000 / aulos::kNoteTable[aulos::Note::C0]; // Sawtooth wave with lowest frequency at highest supported sampling rate.
		Shaper shaper{ kAmplitude, kDeltaY, kDeltaX, shapeParameter, 0 };
		for (float i = 0; i < kDeltaX; ++i)
		{
			INFO("X = " << i << " / " << kDeltaX);
			const auto nextValue = shaper.advance();
			CHECK(std::abs(nextValue) <= kAmplitude);
			CHECK(nextValue == doctest::Approx{ Shaper::value(kAmplitude, kDeltaY, kDeltaX, shapeParameter, i) }.epsilon(epsilon));
			CHECK(nextValue == doctest::Approx{ Shaper{ kAmplitude, kDeltaY, kDeltaX, shapeParameter, i }.advance() }.epsilon(epsilon));
		}
	}
}

TEST_CASE("shaper_cosine")
{
	::checkShaper<aulos::CosineShaper>({}, kPrecision);
}

TEST_CASE("shaper_cubic")
{
	::checkShaper<aulos::CubicShaper>({}, kPrecision);
}

TEST_CASE("shaper_linear")
{
	::checkShaper<aulos::LinearShaper>({}, kPrecision);
}

TEST_CASE("shaper_quadratic1")
{
	::checkShaper<aulos::Quadratic1Shaper>({}, kPrecision);
}

TEST_CASE("shaper_quadratic2")
{
	::checkShaper<aulos::Quadratic2Shaper>({}, kPrecision);
}

TEST_CASE("shaper_quintic")
{
	for (const auto shape : { -1.f, 0.f, 1.f })
	{
		INFO("S = " << shape);
		::checkShaper<aulos::QuinticShaper>(shape, 0.00'001);
	}
}
