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
#include <array>
#include <cmath>
#include <vector>

namespace
{
	constexpr auto kSampleSize = sizeof(float);
	constexpr auto kNoteRatio = 1.0594630943592952645618252949463;

	enum class Note : uint8_t
	{
		// clang-format off
		C0, Db0, D0, Eb0, E0, F0, Gb0, G0, Ab0, A0, Bb0, B0,
		C1, Db1, D1, Eb1, E1, F1, Gb1, G1, Ab1, A1, Bb1, B1,
		C2, Db2, D2, Eb2, E2, F2, Gb2, G2, Ab2, A2, Bb2, B2,
		C3, Db3, D3, Eb3, E3, F3, Gb3, G3, Ab3, A3, Bb3, B3,
		C4, Db4, D4, Eb4, E4, F4, Gb4, G4, Ab4, A4, Bb4, B4,
		C5, Db5, D5, Eb5, E5, F5, Gb5, G5, Ab5, A5, Bb5, B5,
		C6, Db6, D6, Eb6, E6, F6, Gb6, G6, Ab6, A6, Bb6, B6,
		C7, Db7, D7, Eb7, E7, F7, Gb7, G7, Ab7, A7, Bb7, B7,
		C8, Db8, D8, Eb8, E8, F8, Gb8, G8, Ab8, A8, Bb8, B8,
		C9, Db9, D9, Eb9, E9, F9, Gb9, G9, Ab9, A9, Bb9, B9,
		// clang-format on
	};

	class NoteTable
	{
	public:
		NoteTable() noexcept
		{
			_frequencies[static_cast<size_t>(Note::A0)] = 27.5; // A0 for the standard A440 pitch (A4 = 440 Hz).
			for (auto a = static_cast<size_t>(Note::A1); a <= static_cast<size_t>(Note::A9); a += 12)
				_frequencies[a] = _frequencies[a - 12] * 2.0;
			for (size_t base = 0; base < _frequencies.size(); base += 12)
			{
				const auto a = base + static_cast<size_t>(Note::A0) - static_cast<size_t>(Note::C0);
				for (auto note = a; note > base; --note)
					_frequencies[note - 1] = _frequencies[note] / kNoteRatio;
				for (auto note = a; note < base + static_cast<size_t>(Note::B0) - static_cast<size_t>(Note::C0); ++note)
					_frequencies[note + 1] = _frequencies[note] * kNoteRatio;
			}
		}

		constexpr double operator[](Note note) const noexcept
		{
			return _frequencies[static_cast<size_t>(note)];
		}

	private:
		std::array<double, 120> _frequencies;
	};

	const NoteTable kNoteTable;

	struct NoteInfo
	{
		const double _frequency = 0.0;
		const size_t _length = 1;

		constexpr NoteInfo(size_t length = 1) noexcept
			: _length{ length } {}
		constexpr NoteInfo(Note note, size_t length = 1) noexcept
			: _frequency{ kNoteTable[note] }, _length{ length } {}
	};

	class Voice
	{
	public:
		constexpr Voice(size_t samplingRate) noexcept
			: _samplingRate{ samplingRate } {}

		size_t generate(float* base, size_t maxSamples) noexcept
		{
			const auto count = std::min(_samplesToEnd, maxSamples);
			_samplesToEnd -= count;
			if (_frequency >= 1.0)
			{
				const auto samplesPerHalfPeriod = _samplingRate / (2 * _frequency);
				auto remainingHalfPeriod = static_cast<size_t>(_halfPeriodRemaining * samplesPerHalfPeriod);
				size_t remainingSamples = count;
				while (remainingHalfPeriod <= remainingSamples)
				{
					const auto samples = std::min(remainingHalfPeriod, _samplesToSilence);
					for (auto i = samples; i > 0; --i)
						*base++ += _amplitude * (static_cast<float>(_samplesToSilence - i) / _samplingRate);
					remainingSamples -= remainingHalfPeriod;
					_samplesToSilence -= samples;
					_amplitude = -_amplitude;
					remainingHalfPeriod = static_cast<size_t>(samplesPerHalfPeriod);
				}
				const auto samples = std::min(remainingSamples, _samplesToSilence);
				for (auto i = samples; i > 0; --i)
					*base++ += _amplitude * (static_cast<float>(_samplesToSilence - i) / _samplingRate);
				_samplesToSilence -= samples;
				_halfPeriodRemaining = static_cast<double>(remainingHalfPeriod - remainingSamples) / samplesPerHalfPeriod;
			}
			return count;
		}

		void start(double frequency, float amplitude, double duration, size_t samples) noexcept
		{
			_frequency = frequency;
			_amplitude = std::copysign(std::clamp(amplitude, -1.f, 1.f), _amplitude);
			_samplesToSilence = static_cast<size_t>(_samplingRate * duration);
			_samplesToEnd = samples;
		}

	private:
		const size_t _samplingRate;
		double _frequency = 1.0;
		float _amplitude = 1.f;
		double _halfPeriodRemaining = 1.0;
		size_t _samplesToSilence = 0;
		size_t _samplesToEnd = 0;
	};
}

namespace aulos
{
	class RendererImpl
	{
	public:
		RendererImpl(unsigned samplingRate)
			: _samplingRate{ samplingRate } {}

	public:
		const unsigned _samplingRate;
		const size_t _stepSamples = _samplingRate / 4;
		Voice _voice{ _samplingRate };
		std::vector<NoteInfo> _notes;
		std::vector<NoteInfo>::const_iterator _currentNote;
	};

	Renderer::Renderer(unsigned samplingRate)
		: _impl{ std::make_unique<RendererImpl>(samplingRate) }
	{
		_impl->_notes.emplace_back(1);
		_impl->_notes.emplace_back(Note::E5);
		_impl->_notes.emplace_back(Note::Eb5);
		_impl->_notes.emplace_back(Note::E5);
		_impl->_notes.emplace_back(Note::Eb5);
		_impl->_notes.emplace_back(Note::E5);
		_impl->_notes.emplace_back(Note::B4);
		_impl->_notes.emplace_back(Note::D5);
		_impl->_notes.emplace_back(Note::C5);
		_impl->_notes.emplace_back(Note::A4, 3);
		_impl->_notes.emplace_back(Note::C4);
		_impl->_notes.emplace_back(Note::E4);
		_impl->_notes.emplace_back(Note::A4);
		_impl->_notes.emplace_back(Note::B4, 3);
		_impl->_notes.emplace_back(Note::E4);
		_impl->_notes.emplace_back(Note::A4);
		_impl->_notes.emplace_back(Note::B4);
		_impl->_notes.emplace_back(Note::C5, 3);
		_impl->_notes.emplace_back(Note::E4);
		_impl->_notes.emplace_back(Note::E5);
		_impl->_notes.emplace_back(Note::Eb5);
		_impl->_notes.emplace_back(Note::E5);
		_impl->_notes.emplace_back(Note::Eb5);
		_impl->_notes.emplace_back(Note::E5);
		_impl->_notes.emplace_back(Note::B4);
		_impl->_notes.emplace_back(Note::D5);
		_impl->_notes.emplace_back(Note::C5);
		_impl->_notes.emplace_back(Note::A4, 3);
		_impl->_notes.emplace_back(Note::C4);
		_impl->_notes.emplace_back(Note::E4);
		_impl->_notes.emplace_back(Note::A4);
		_impl->_notes.emplace_back(Note::B4, 3);
		_impl->_notes.emplace_back(Note::E4);
		_impl->_notes.emplace_back(Note::C5);
		_impl->_notes.emplace_back(Note::B4);
		_impl->_notes.emplace_back(Note::A4, 4);
		_impl->_currentNote = _impl->_notes.cbegin();
		if (_impl->_currentNote != _impl->_notes.cend())
			_impl->_voice.start(_impl->_currentNote->_frequency, .25f, 1.0, _impl->_stepSamples * _impl->_currentNote->_length);
	}

	Renderer::~Renderer() noexcept = default;

	size_t Renderer::render(void* buffer, size_t bufferBytes) noexcept
	{
		if (_impl->_currentNote == _impl->_notes.cend())
			return 0;
		std::memset(buffer, 0, bufferBytes);
		size_t samplesRendered = 0;
		for (;;)
		{
			const auto samplesToGenerate = bufferBytes / kSampleSize - samplesRendered;
			if (!samplesToGenerate)
				break;
			const auto samplesGenerated = _impl->_voice.generate(static_cast<float*>(buffer) + samplesRendered, samplesToGenerate);
			samplesRendered += samplesGenerated;
			if (samplesGenerated < samplesToGenerate)
			{
				++_impl->_currentNote;
				if (_impl->_currentNote != _impl->_notes.cend())
					_impl->_voice.start(_impl->_currentNote->_frequency, .25f, 1.0, _impl->_stepSamples * _impl->_currentNote->_length);
			}
		}
		return samplesRendered * kSampleSize;
	}
}
