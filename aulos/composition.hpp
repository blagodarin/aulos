// This file is part of the Aulos toolkit.
// Copyright (C) Sergei Blagodarin.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <memory>

namespace aulos
{
	// Contains audio data in an optimized format.
	class Composition
	{
	public:
		[[nodiscard]] static std::unique_ptr<Composition> create(const char* textSource);

		virtual ~Composition() noexcept = default;
	};

	// Generates PCM audio for a composition.
	class Renderer
	{
	public:
		[[nodiscard]] static std::unique_ptr<Renderer> create(const Composition&, unsigned samplingRate, unsigned channels, bool looping = false);

		virtual ~Renderer() noexcept = default;

		[[nodiscard]] virtual unsigned channels() const noexcept = 0;
		[[nodiscard]] virtual size_t render(void* buffer, size_t bufferBytes) noexcept = 0;
		virtual void restart() noexcept = 0;
		[[nodiscard]] virtual unsigned samplingRate() const noexcept = 0;
		[[nodiscard]] virtual size_t totalSamples() const noexcept = 0;
	};
}
