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
#include <numeric>

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
	SampledEnvelope::SampledEnvelope(const Envelope& envelope, size_t samplingRate) noexcept
	{
		assert(envelope._changes.size() < _points.size());
		_points[_size++] = { 0, envelope._initial };
		for (const auto& point : envelope._changes)
		{
			assert(point._delay >= 0.f);
			if (point._delay > 0.f)
				_points[_size++] = { static_cast<size_t>(double{ point._delay } * samplingRate), point._value };
			else
				_points[_size]._value = point._value;
		}
	}

	size_t SampledEnvelope::duration() const noexcept
	{
		return std::accumulate(_points.begin(), _points.end(), size_t{}, [](size_t duration, const Point& point) {
			return duration + point._delay;
		});
	}

	LinearModulator::LinearModulator(const SampledEnvelope& envelope) noexcept
		: _envelope{ envelope }
	{
	}

	void LinearModulator::advance(size_t samples) noexcept
	{
		while (_nextPoint != _envelope.end())
		{
			const auto remainingSamples = _nextPoint->_delay - _offset;
			if (remainingSamples > samples)
			{
				_offset += samples;
				const auto progress = static_cast<double>(_offset) / _nextPoint->_delay;
				_currentValue = (_baseValue * (1.0 - progress) + _nextPoint->_value * progress);
				break;
			}
			samples -= remainingSamples;
			_baseValue = _nextPoint->_value;
			++_nextPoint;
			_offset = 0;
			_currentValue = _baseValue;
		}
	}

	size_t LinearModulator::maxContinuousAdvance() const noexcept
	{
		assert(_nextPoint != _envelope.end());
		return _nextPoint->_delay - _offset;
	}

	void LinearModulator::start(bool fromCurrent) noexcept
	{
		assert(_envelope.size() > 0);
		assert(_envelope.begin()->_delay == 0);
		_nextPoint = std::next(_envelope.begin());
		assert(_nextPoint == _envelope.end() || _nextPoint->_delay > 0);
		_baseValue = fromCurrent ? _currentValue : _envelope.begin()->_value;
		_offset = 0;
		_currentValue = _baseValue;
	}

	double LinearModulator::valueStep() const noexcept
	{
		if (_nextPoint == _envelope.end())
			return 0;
		assert(_offset < _nextPoint->_delay);
		return (_nextPoint->_value - _currentValue) / (_nextPoint->_delay - _offset);
	}

	Modulator::Modulator(const VoiceData& data, unsigned samplingRate) noexcept
		: _amplitudeEnvelope{ data._amplitudeEnvelope, samplingRate }
		, _frequencyEnvelope{ data._frequencyEnvelope, samplingRate }
		, _asymmetryEnvelope{ data._asymmetryEnvelope, samplingRate }
		, _oscillationEnvelope{ data._oscillationEnvelope, samplingRate }
	{
	}

	void Modulator::advance(size_t samples) noexcept
	{
		_amplitudeModulator.advance(samples);
		_frequencyModulator.advance(samples);
		_asymmetryModulator.advance(samples);
		_oscillationModulator.advance(samples);
	}

	void Modulator::start() noexcept
	{
		_amplitudeModulator.start(!_amplitudeModulator.stopped());
		_frequencyModulator.start(false);
		_asymmetryModulator.start(false);
		_oscillationModulator.start(false);
	}

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

	void Oscillator::restart(double frequency, double asymmetry) noexcept
	{
		_amplitudeSign = 1.f;
		resetStage(frequency, asymmetry);
		_stageRemainder = _stageLength;
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
