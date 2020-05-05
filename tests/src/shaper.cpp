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
	template <typename Shaper>
	void checkShaper(float shapeParameter, int precisionBits)
	{
		constexpr auto amplitude = 1.f;
		constexpr auto range = 2 * amplitude;
		const auto precision = std::ldexp(range, -precisionBits);
		constexpr auto minFrequency = aulos::kNoteTable[aulos::Note::C0] / 2; // Lowest note at lowest frequency modulation.
		constexpr auto deltaX = 48'000 / minFrequency;                        // Asymmetric wave of minimum frequency at highest supported sampling rate.
		Shaper shaper{ amplitude, -range, deltaX, shapeParameter, 0 };
		for (float i = 0; i < deltaX; ++i)
		{
			INFO("X = " << i << " / " << deltaX);
			const auto expected = Shaper::template value<double>(amplitude, -range, deltaX, shapeParameter, i);
			const auto initialValue = Shaper{ amplitude, -range, deltaX, shapeParameter, i }.advance();
			CHECK(std::abs(initialValue) <= amplitude);
			CHECK(initialValue == doctest::Approx{ expected }.epsilon(precision));
			const auto advancedValue = shaper.advance();
			CHECK(std::abs(advancedValue) <= amplitude);
			CHECK(advancedValue == doctest::Approx{ expected }.epsilon(precision));
		}
	}
}

TEST_CASE("shaper_cosine")
{
	::checkShaper<aulos::CosineShaper>({}, 23);
}

TEST_CASE("shaper_cubic")
{
	::checkShaper<aulos::CubicShaper>({}, 23);
}

TEST_CASE("shaper_linear")
{
	::checkShaper<aulos::LinearShaper>({}, 23);
}

TEST_CASE("shaper_quadratic1")
{
	::checkShaper<aulos::Quadratic1Shaper>({}, 23);
}

TEST_CASE("shaper_quadratic2")
{
	::checkShaper<aulos::Quadratic2Shaper>({}, 23);
}

TEST_CASE("shaper_quintic")
{
	for (const auto shape : { -1.f, 0.f, 1.f })
	{
		INFO("S = " << shape);
		::checkShaper<aulos::QuinticShaper>(shape, 18);
	}
}
