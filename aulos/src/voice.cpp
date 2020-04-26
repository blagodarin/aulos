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

#include "voice.hpp"

#include <algorithm>
#include <cassert>

namespace
{
	constexpr auto kNoteRatio = 1.0594630943592952645618252949463;

	class NoteTable
	{
	public:
		NoteTable() noexcept
		{
			_frequencies[static_cast<size_t>(aulos::Note::A0)] = 27.5; // A0 for the standard A440 pitch (A4 = 440 Hz).
			for (auto a = static_cast<size_t>(aulos::Note::A1); a <= static_cast<size_t>(aulos::Note::A9); a += 12)
				_frequencies[a] = _frequencies[a - 12] * 2.0;
			for (size_t base = static_cast<size_t>(aulos::Note::C0); base < _frequencies.size() - 1; base += 12)
			{
				const auto a = base + static_cast<size_t>(aulos::Note::A0) - static_cast<size_t>(aulos::Note::C0);
				for (auto note = a; note > base; --note)
					_frequencies[note - 1] = _frequencies[note] / kNoteRatio;
				for (auto note = a; note < base + static_cast<size_t>(aulos::Note::B0) - static_cast<size_t>(aulos::Note::C0); ++note)
					_frequencies[note + 1] = _frequencies[note] * kNoteRatio;
			}
		}

		constexpr double operator[](aulos::Note note) const noexcept
		{
			return _frequencies[static_cast<size_t>(note)];
		}

	private:
		std::array<double, 120> _frequencies;
	};

	const NoteTable kNoteTable;
}

namespace aulos
{
	void Oscillator::adjustStage(double frequency, double asymmetry) noexcept
	{
		const auto partRatio = _stageRemainder / _stageLength;
		resetStage(frequency, asymmetry);
		_stageRemainder = _stageLength * partRatio;
	}

	void Oscillator::advance(size_t samples, double nextFrequency, double nextAsymmetry) noexcept
	{
		auto remaining = _stageRemainder - samples;
		assert(remaining > -1);
		while (remaining <= 0)
		{
			_amplitudeSign = -_amplitudeSign;
			resetStage(nextFrequency, nextAsymmetry);
			remaining += _stageLength;
		}
		_stageRemainder = remaining;
	}

	void Oscillator::restart(double frequency, double asymmetry, double shift) noexcept
	{
		_amplitudeSign = 1.f;
		resetStage(frequency, asymmetry);
		auto remainder = _stageLength;
		assert(shift >= 0);
		if (shift > 0)
			for (remainder -= _samplingRate * shift; remainder <= 0; remainder += _stageLength)
			{
				_amplitudeSign = -_amplitudeSign;
				resetStage(frequency, asymmetry);
			}
		_stageRemainder = remainder;
	}

	void Oscillator::resetStage(double frequency, double asymmetry) noexcept
	{
		assert(frequency > 0);
		assert(asymmetry >= -1 && asymmetry <= 1);
		auto orientedAsymmetry = _amplitudeSign * asymmetry;
		if (orientedAsymmetry == -1)
		{
			_amplitudeSign = -_amplitudeSign;
			orientedAsymmetry = 1;
		}
		const auto partLength = _samplingRate * (1 + orientedAsymmetry) / (2 * frequency);
		assert(partLength > 0);
		_stageLength = partLength;
	}

	void VoiceImpl::startImpl(Note note, float amplitude) noexcept
	{
		_baseFrequency = kNoteTable[note];
		_baseAmplitude = std::clamp(amplitude, -1.f, 1.f);
	}
}
