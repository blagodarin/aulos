// This file is part of the Aulos toolkit.
// Copyright (C) Sergei Blagodarin.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <memory>

namespace aulos
{
	// Contains packed audio data.
	class Composition
	{
	public:
		// Loads a composition from text data.
		[[nodiscard]] static std::unique_ptr<Composition> create(const char* textData);

		virtual ~Composition() noexcept = default;
	};
}
