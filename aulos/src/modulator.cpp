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
#include <limits>
#include <numeric>

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
		return _nextPoint != _envelope.end() ? _nextPoint->_delay - _offset : std::numeric_limits<size_t>::max();
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
}
