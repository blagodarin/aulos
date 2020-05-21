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
		[[nodiscard]] static std::unique_ptr<Renderer> create(const Composition&, unsigned samplingRate, unsigned channels);

		virtual ~Renderer() noexcept = default;

		[[nodiscard]] virtual unsigned channels() const noexcept = 0;
		[[nodiscard]] virtual size_t render(void* buffer, size_t bufferBytes) noexcept = 0;
		virtual void restart() noexcept = 0;
		[[nodiscard]] virtual unsigned samplingRate() const noexcept = 0;
		[[nodiscard]] size_t totalBytes() const noexcept { return totalSamples() * channels() * sizeof(float); }
		[[nodiscard]] virtual size_t totalSamples() const noexcept = 0;
	};
}
