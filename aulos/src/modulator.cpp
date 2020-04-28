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

#include "modulator.hpp"

#include <cassert>

namespace aulos
{
	SampledEnvelope::SampledEnvelope(const Envelope& envelope, unsigned samplingRate) noexcept
	{
		assert(envelope._changes.size() < _points.size());
		size_t i = 0;
		_points[i] = { 0, envelope._initial };
		for (const auto& point : envelope._changes)
		{
			assert(point._delay >= 0.f);
			if (point._delay > 0.f)
				_points[++i] = { static_cast<unsigned>(point._delay * samplingRate), point._value };
			else
				_points[i]._value = point._value;
		}
		_end = &_points[i + 1];
	}

	void LinearModulator::advance(unsigned samples) noexcept
	{
		if (_nextPoint == _envelope.end())
			return;
		const auto remainingSamples = _nextPoint->_delay - _offset;
		if (samples < remainingSamples)
		{
			_offset += samples;
			_currentValue = _baseValue + _offset * _step;
			return;
		}
		assert(samples == remainingSamples);
		_baseValue = _nextPoint->_value;
		++_nextPoint;
		_step = _nextPoint != _envelope.end() ? (_nextPoint->_value - _baseValue) / _nextPoint->_delay : 0.f;
		_offset = 0;
		_currentValue = _baseValue;
	}

	void LinearModulator::start(bool fromCurrent) noexcept
	{
		assert(_envelope.begin() != _envelope.end());
		assert(_envelope.begin()->_delay == 0);
		_nextPoint = std::next(_envelope.begin());
		assert(_nextPoint == _envelope.end() || _nextPoint->_delay > 0);
		_baseValue = fromCurrent ? _currentValue : _envelope.begin()->_value;
		_step = _nextPoint != _envelope.end() ? (_nextPoint->_value - _baseValue) / _nextPoint->_delay : 0.f;
		_offset = 0;
		_currentValue = _baseValue;
	}

	ModulationData::ModulationData(const VoiceData& voice, unsigned samplingRate) noexcept
		: _amplitudeEnvelope{ voice._amplitudeEnvelope, samplingRate }
		, _frequencyEnvelope{ voice._frequencyEnvelope, samplingRate }
		, _asymmetryEnvelope{ voice._asymmetryEnvelope, samplingRate }
		, _oscillationEnvelope{ voice._oscillationEnvelope, samplingRate }
	{
	}

	Modulator::Modulator(const ModulationData& data) noexcept
		: _amplitudeModulator{ data._amplitudeEnvelope }
		, _frequencyModulator{ data._frequencyEnvelope }
		, _asymmetryModulator{ data._asymmetryEnvelope }
		, _oscillationModulator{ data._oscillationEnvelope }
	{
	}

	void Modulator::advance(unsigned samples) noexcept
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
}
