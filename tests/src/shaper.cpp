// This file is part of the Aulos toolkit.
// Copyright (C) Sergei Blagodarin.
// SPDX-License-Identifier: Apache-2.0

#include <aulos/src/shaper.hpp> // Included first to check header consistency.

#include <aulos/composition.hpp>

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
		constexpr auto minFrequency = aulos::kNoteTable[aulos::Note::C0] / 2;     // Lowest note at lowest frequency modulation.
		constexpr auto deltaX = aulos::Renderer::kMaxSamplingRate / minFrequency; // Asymmetric wave of minimum frequency at highest supported sampling rate.
		Shaper shaper{ { amplitude, -range, deltaX, shapeParameter } };
		for (float i = 0; i < deltaX; ++i)
		{
			INFO("X = " << i << " / " << deltaX);
			const auto expected = Shaper::template value<double>(amplitude, -range, deltaX, shapeParameter, i);
			const auto initialValue = Shaper{ { amplitude, -range, deltaX, shapeParameter, i } }.advance();
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

TEST_CASE("shaper_linear")
{
	::checkShaper<aulos::LinearShaper>({}, 23);
}

TEST_CASE("shaper_sharp_quadratic")
{
	::checkShaper<aulos::SharpQuadraticShaper>({}, 23);
}

TEST_CASE("shaper_smooth_cubic")
{
	for (const auto shape : { 0.f, 1.f, 2.f, 3.f })
	{
		INFO("S = " << shape);
		::checkShaper<aulos::SmoothCubicShaper>(shape, 23);
	}
}

TEST_CASE("shaper_smooth_quadratic")
{
	::checkShaper<aulos::SmoothQuadraticShaper>({}, 23);
}

TEST_CASE("shaper_quintic")
{
	for (const auto shape : { -1.f, 0.f, 1.f })
	{
		INFO("S = " << shape);
		::checkShaper<aulos::QuinticShaper>(shape, 18);
	}
}
