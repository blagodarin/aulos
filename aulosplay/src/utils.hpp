// This file is part of the Aulos toolkit.
// Copyright (C) Sergei Blagodarin.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <cstddef>

namespace aulosplay
{
	void monoToStereo(float* output, const float* input, size_t frames) noexcept;
}
