// This file is part of the Aulos toolkit.
// Copyright (C) Sergei Blagodarin.
// SPDX-License-Identifier: Apache-2.0

#include <aulos/src/oscillator.hpp>

#include <doctest.h>

TEST_CASE("TriangleOscillator")
{
	aulos::TriangleOscillator oscillator{ .25f, 1.f };
	CHECK(oscillator.value(0.f) == 1.f);
	CHECK(oscillator.value(1.f) == .5f);
	CHECK(oscillator.value(2.f) == 0.f);
	CHECK(oscillator.value(3.f) == .5f);
	CHECK(oscillator.value(std::nextafter(4.f, 0.f)) == doctest::Approx{ 1.f });
	CHECK(oscillator.value(4.f) == 1.f);
	CHECK(oscillator.value(5.f) == .5f);
}
