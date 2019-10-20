//
// This file is part of the Aulos toolkit.
// Copyright (C) 2019 Sergei Blagodarin.
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

#include <aulos.hpp>

#include <algorithm>
#include <cmath>

namespace aulos
{
	class RendererImpl
	{
	public:
		size_t _bytesWritten = 0;
	};

	Renderer::Renderer(const void*, size_t)
		: _impl{ std::make_unique<RendererImpl>() }
	{
	}

	Renderer::~Renderer() noexcept = default;

	size_t Renderer::render(void* buffer, size_t bufferBytes) noexcept
	{
		constexpr size_t samplingRate = 48'000;
		constexpr auto totalSamples = samplingRate / 2;
		constexpr auto totalSize = totalSamples * 4;
		constexpr auto timeStep = 440.0 / samplingRate;
		const auto samplesWritten = _impl->_bytesWritten / 4;
		const auto nextSamplesWritten = samplesWritten + std::min(totalSamples - samplesWritten, bufferBytes / 4);
		for (size_t i = samplesWritten; i < nextSamplesWritten; ++i)
		{
			const auto base = std::sin(2 * M_PI * timeStep * static_cast<double>(i));
			const auto amplitude = static_cast<double>(totalSamples - i) / totalSamples;
			static_cast<float*>(buffer)[i - samplesWritten] = static_cast<float>(base * amplitude);
		}
		_impl->_bytesWritten = nextSamplesWritten * 4;
		return (nextSamplesWritten - samplesWritten) * 4;
	}
}
