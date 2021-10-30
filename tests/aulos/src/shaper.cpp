// This file is part of the Aulos toolkit.
// Copyright (C) Sergei Blagodarin.
// SPDX-License-Identifier: Apache-2.0

#include <aulos/src/shaper.hpp> // Included first to check header consistency.

#include <aulos/renderer.hpp>

#include <aulos/src/tables.hpp>

#include <doctest/doctest.h>

namespace
{
	template <typename Shaper>
	void checkShaper(float shapeParameter, int precisionBits)
	{
		INFO("Shape = " << shapeParameter << ", Precision = " << precisionBits);
		constexpr auto amplitude = 1.f;
		constexpr auto range = 2 * amplitude;
		const auto precision = std::ldexp(range, -precisionBits);
		const auto minFrequency = aulos::kNoteFrequencies[aulos::Note::C0] / 2; // Lowest note at lowest frequency modulation.
		const auto deltaX = aulos::Renderer::kMaxSamplingRate / minFrequency; // Asymmetric wave of minimum frequency at highest supported sampling rate.
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

TEST_CASE("shaper_cubic")
{
	::checkShaper<aulos::CubicShaper>(0.f, 23);
	::checkShaper<aulos::CubicShaper>(3.f, 22);
	::checkShaper<aulos::CubicShaper>(8.98f, 20);
}

TEST_CASE("shaper_linear")
{
	::checkShaper<aulos::LinearShaper>({}, 23);
}

TEST_CASE("shaper_quadratic")
{
	::checkShaper<aulos::QuadraticShaper>(0.f, 23);
	::checkShaper<aulos::QuadraticShaper>(3.f, 23);
	::checkShaper<aulos::QuadraticShaper>(5.f, 21);
	::checkShaper<aulos::QuadraticShaper>(6.82f, 20);
}

TEST_CASE("shaper_quintic")
{
	::checkShaper<aulos::QuinticShaper>(-1.5f, 23);
	::checkShaper<aulos::QuinticShaper>(-1.f, 20);
	::checkShaper<aulos::QuinticShaper>(0.f, 19);
	::checkShaper<aulos::QuinticShaper>(1.f, 18);
	::checkShaper<aulos::QuinticShaper>(3.f, 17);
	::checkShaper<aulos::QuinticShaper>(4.01f, 16);
}
