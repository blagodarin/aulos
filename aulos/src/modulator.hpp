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

#include <aulos/voice.hpp>

#include "shaper.hpp"

#include <array>
#include <cassert>
#include <limits>
#include <numeric>

namespace aulos
{
	struct SampledPoint
	{
		unsigned _delaySamples;
		float _value;

		constexpr SampledPoint(unsigned delaySamples, float value) noexcept
			: _delaySamples{ delaySamples }, _value{ value } {}
	};

	// MSVC 16.5.4 doesn't have std::span.
	struct SampledPoints
	{
		const SampledPoint* _data;
		unsigned _size;
	};

	class Modulator
	{
	public:
		explicit constexpr Modulator(const SampledPoints& points) noexcept
			: _points{ points._data }
			, _size{ points._size }
		{
		}

		constexpr void advance(unsigned samples) noexcept
		{
			while (_nextIndex < _size)
			{
				const auto remainingDelay = _points[_nextIndex]._delaySamples - _offsetSamples;
				if (remainingDelay > samples)
				{
					_offsetSamples += samples;
					break;
				}
				samples -= remainingDelay;
				_lastPointValue = _points[_nextIndex++]._value;
				_offsetSamples = 0;
			}
		}

		constexpr ShaperData shaperData() const noexcept
		{
			return { _lastPointValue, _points[_nextIndex]._value - _lastPointValue, static_cast<float>(_points[_nextIndex]._delaySamples), 0, static_cast<float>(_offsetSamples) };
		}

		constexpr auto currentBaseValue() const noexcept
		{
			return _lastPointValue;
		}

		constexpr auto currentOffset() const noexcept
		{
			return _offsetSamples;
		}

		template <typename Shaper>
		auto currentValue() const noexcept
		{
			return Shaper::value(_lastPointValue, _points[_nextIndex]._value - _lastPointValue, static_cast<float>(_points[_nextIndex]._delaySamples), 0.f, static_cast<float>(_offsetSamples));
		}

		constexpr auto maxContinuousAdvance() const noexcept
		{
			return _points[_nextIndex]._delaySamples - _offsetSamples;
		}

		void start(float initialValue) noexcept
		{
			_lastPointValue = initialValue;
			_nextIndex = 0;
			while (_nextIndex < _size && !_points[_nextIndex]._delaySamples)
				_lastPointValue = _points[_nextIndex++]._value;
			_offsetSamples = 0;
		}

		constexpr void stop() noexcept
		{
			_lastPointValue = _points[_size]._value;
			_nextIndex = _size;
			_offsetSamples = 0;
		}

		constexpr bool stopped() const noexcept
		{
			return _nextIndex == _size;
		}

		auto totalSamples() const noexcept
		{
			// MSVC 16.5.4 doesn't have constexpr std::accumulate.
			return std::accumulate(_points, _points + _size, size_t{}, [](size_t result, const SampledPoint& point) { return result + point._delaySamples; });
		}

	private:
		const SampledPoint* const _points;
		const unsigned _size;
		float _lastPointValue{ _points[_size]._value };
		unsigned _nextIndex = _size;
		unsigned _offsetSamples = 0;
	};
}
