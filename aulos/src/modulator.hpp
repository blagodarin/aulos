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

#include <aulos/data.hpp>

#include <array>
#include <cassert>
#include <limits>
#include <numeric>

namespace aulos
{
	struct SampledPoint
	{
		unsigned _delay;
		float _value;
	};

	struct SampledEnvelope
	{
	public:
		SampledEnvelope(const Envelope& envelope, unsigned samplingRate) noexcept
		{
			_points[_size++] = { 0, envelope._initial };
			for (const auto& point : envelope._changes)
			{
				assert(point._delay >= 0.f);
				if (point._delay > 0.f)
				{
					assert(_size < _points.size());
					_points[_size++] = { static_cast<unsigned>(point._delay * samplingRate), point._value };
				}
				else
					_points[_size - 1]._value = point._value;
			}
		}

		constexpr auto data() const noexcept
		{
			return _points.data();
		}

		constexpr auto size() const noexcept
		{
			return _size;
		}

	private:
		std::array<SampledPoint, 5> _points{};
		unsigned _size = 0;
	};

	class LinearModulator
	{
	public:
		LinearModulator(const SampledEnvelope& envelope) noexcept
			: _points{ envelope.data() }
			, _size{ envelope.size() }
		{
			assert(_size > 0);
			assert(_points[0]._delay == 0);
			_currentValue = _points[_size - 1]._value;
		}

		void advance(unsigned samples) noexcept
		{
			while (_index < _size)
			{
				const auto remainingDelay = _points[_index]._delay - _offset;
				if (remainingDelay > samples)
				{
					_offset += samples;
					_currentValue += samples * _step;
					break;
				}
				samples -= remainingDelay;
				_currentValue = _points[_index]._value;
				++_index;
				_step = _index < _size ? (_points[_index]._value - _currentValue) / _points[_index]._delay : 0.f;
				_offset = 0;
			}
		}

		constexpr auto maxContinuousAdvance() const noexcept
		{
			return _index < _size ? _points[_index]._delay - _offset : std::numeric_limits<unsigned>::max();
		}

		void start(bool fromCurrent) noexcept
		{
			_index = 1;
			assert(_index == _size || _points[_index]._delay > 0);
			if (!fromCurrent)
				_currentValue = _points[0]._value;
			_step = _index < _size ? (_points[_index]._value - _currentValue) / _points[_index]._delay : 0.f;
			_offset = 0;
		}

		constexpr void stop() noexcept
		{
			_index = _size;
		}

		constexpr bool stopped() const noexcept
		{
			return _index == _size;
		}

		auto totalSamples() const noexcept
		{
			// std::accumulate is not constexpr in MSVC 2019 (16.5.4).
			return std::accumulate(_points, _points + _size, size_t{}, [](size_t result, const SampledPoint& point) { return result + point._delay; });
		}

		constexpr auto value() const noexcept
		{
			return _currentValue;
		}

		constexpr auto valueStep() const noexcept
		{
			return _step;
		}

	private:
		const SampledPoint* const _points;
		const unsigned _size;
		unsigned _index = _size;
		float _step = 0.f;
		unsigned _offset = 0;
		float _currentValue = 0.f;
	};

	struct ModulationData
	{
		SampledEnvelope _amplitudeEnvelope;
		SampledEnvelope _frequencyEnvelope;
		SampledEnvelope _asymmetryEnvelope;
		SampledEnvelope _oscillationEnvelope;

		ModulationData(const VoiceData&, unsigned samplingRate) noexcept;
	};

	class Modulator
	{
	public:
		Modulator(const ModulationData&) noexcept;

		void advance(unsigned samples) noexcept;
		constexpr auto currentAmplitude() const noexcept { return _amplitudeModulator.value(); }
		constexpr auto currentAmplitudeStep() const noexcept { return _amplitudeModulator.valueStep(); }
		constexpr auto currentAsymmetry() const noexcept { return _asymmetryModulator.value(); }
		constexpr auto currentFrequency() const noexcept { return _frequencyModulator.value(); }
		constexpr auto currentOscillation() const noexcept { return _oscillationModulator.value(); }
		constexpr auto maxAdvance() const noexcept { return _amplitudeModulator.maxContinuousAdvance(); }
		void start() noexcept;
		constexpr void stop() noexcept { _amplitudeModulator.stop(); }
		constexpr auto stopped() const noexcept { return _amplitudeModulator.stopped(); }
		auto totalSamples() const noexcept { return _amplitudeModulator.totalSamples(); }

	private:
		LinearModulator _amplitudeModulator;
		LinearModulator _frequencyModulator;
		LinearModulator _asymmetryModulator;
		LinearModulator _oscillationModulator;
	};
}
