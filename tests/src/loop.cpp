// This file is part of the Aulos toolkit.
// Copyright (C) Sergei Blagodarin.
// SPDX-License-Identifier: Apache-2.0

#include <aulos/composition.hpp>
#include <aulos/data.hpp>

#include <doctest.h>

TEST_CASE("composition_has_loop")
{
	aulos::CompositionData composition;
	REQUIRE(composition._loopLength == 0);
	CHECK(!composition.pack()->hasLoop());
	composition._loopLength = 1;
	CHECK(composition.pack()->hasLoop());
}
