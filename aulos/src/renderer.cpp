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
#include <vector>

namespace
{
	constexpr auto kSampleSize = sizeof(float);

	struct Note
	{
		const double _frequency;
		const double _gain;
		const double _duration;

		constexpr Note(double frequency, double gain, double duration = 1.0) noexcept
			: _frequency{ frequency }, _gain{ gain }, _duration{ duration } {}
	};
}

namespace aulos
{
	class RendererImpl
	{
	public:
		std::vector<Note> _notes;
		std::vector<Note>::const_iterator _currentNote;
		size_t _currentNoteOffset = 0;
	};

	Renderer::Renderer()
		: _impl{ std::make_unique<RendererImpl>() }
	{
		_impl->_notes.emplace_back(220.0, 1.0, 0.5);
		_impl->_notes.emplace_back(220.0, 1.0, 0.5);
		_impl->_notes.emplace_back(220.0, 1.0, 0.5);
		_impl->_notes.emplace_back(220.0, 1.0);
		_impl->_notes.emplace_back(440.0, 1.0);
		_impl->_notes.emplace_back(220.0, 1.0);
		_impl->_notes.emplace_back(440.0, 1.0);
		_impl->_notes.emplace_back(220.0, 1.0);
		_impl->_notes.emplace_back(440.0, 1.0, 0.5);
		_impl->_notes.emplace_back(220.0, 1.0);
		_impl->_notes.emplace_back(440.0, 1.0, 0.5);
		_impl->_notes.emplace_back(220.0, 1.0, 0.5);
		_impl->_notes.emplace_back(440.0, 1.0, 0.5);
		_impl->_notes.emplace_back(220.0, 1.0);
		_impl->_notes.emplace_back(440.0, 1.0);
		_impl->_notes.emplace_back(220.0, 0.5);
		_impl->_notes.emplace_back(440.0, 0.5);
		_impl->_notes.emplace_back(220.0, 0.25);
		_impl->_notes.emplace_back(440.0, 0.25, 0.5);
		_impl->_notes.emplace_back(220.0, 0.125);
		_impl->_notes.emplace_back(440.0, 0.125);
		_impl->_currentNote = _impl->_notes.cbegin();
	}

	Renderer::~Renderer() noexcept = default;

	size_t Renderer::render(void* buffer, size_t bufferBytes) noexcept
	{
		constexpr size_t samplingRate = 48'000;
		constexpr auto baseNoteSamples = samplingRate / 4;

		size_t samplesWritten = 0;
		while (_impl->_currentNote != _impl->_notes.cend())
		{
			const auto totalNoteSamples = static_cast<size_t>(baseNoteSamples * _impl->_currentNote->_duration);
			const auto remainingSamples = totalNoteSamples - _impl->_currentNoteOffset;
			if (!remainingSamples)
			{
				++_impl->_currentNote;
				_impl->_currentNoteOffset = 0;
				continue;
			}
			const auto samplesToWrite = std::min(remainingSamples, bufferBytes / kSampleSize - samplesWritten);
			if (!samplesToWrite)
				break;
			const auto timeStep = _impl->_currentNote->_frequency / samplingRate;
			auto sample = static_cast<float*>(buffer) + samplesWritten;
			for (auto i = _impl->_currentNoteOffset; i < _impl->_currentNoteOffset + samplesToWrite; ++i)
			{
				const auto base = std::sin(2 * M_PI * timeStep * static_cast<double>(i));
				const auto amplitude = static_cast<double>(totalNoteSamples - i) / totalNoteSamples;
				*sample++ = static_cast<float>(base * amplitude * _impl->_currentNote->_gain);
			}
			samplesWritten += samplesToWrite;
			_impl->_currentNoteOffset += samplesToWrite;
		}
		return samplesWritten * kSampleSize;
	}
}
