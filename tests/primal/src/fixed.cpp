// This file is part of the Primal library.
// Copyright (C) Sergei Blagodarin.
// SPDX-License-Identifier: Apache-2.0

#include <primal/fixed.hpp>

#include <doctest.h>

TEST_CASE("fixed_u4")
{
	using FixedU4 = primal::Fixed<unsigned, 4>;
	SUBCASE("zero")
	{
		const FixedU4 value;
		CHECK(value.store() == 0);
		CHECK(static_cast<float>(value) == 0.f);
	}
	SUBCASE("one")
	{
		const FixedU4 value{ 1.f };
		CHECK(value.store() == 1 << 4);
		CHECK(static_cast<float>(value) == 1.f);
	}
	SUBCASE("load")
	{
		const auto value = FixedU4::load(1);
		CHECK(value.store() == 1);
		CHECK(static_cast<float>(value) == std::exp2(-4.f));
	}
	SUBCASE("ceil_1")
	{
		const auto value = FixedU4::ceil(std::exp2(-5.f));
		CHECK(value.store() == 1);
		CHECK(static_cast<float>(value) == std::exp2(-4.f));
	}
	SUBCASE("ceil_3")
	{
		const auto value = FixedU4::ceil(5 * std::exp2(-5.f));
		CHECK(value.store() == 3);
		CHECK(static_cast<float>(value) == 1.5f * std::exp2(-3.f));
	}
}
