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
		unsigned _delaySamples;
		float _value;
	};

	struct SampledEnvelope
	{
	public:
		SampledEnvelope(const Envelope& envelope, unsigned samplingRate) noexcept
		{
			for (const auto& point : envelope._points)
			{
				assert(_size < _points.size());
				_points[_size++] = { point._delayMs * samplingRate / 1000, point._value };
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

	class Modulator
	{
	public:
		Modulator(const SampledEnvelope& envelope) noexcept
			: _points{ envelope.data() }
			, _size{ envelope.size() }
		{
		}

		void advance(unsigned samples) noexcept
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

		template <typename Shaper>
		auto createShaper() const noexcept
		{
			return _nextIndex < _size
				? Shaper{ _lastPointValue, _points[_nextIndex]._value - _lastPointValue, static_cast<float>(_points[_nextIndex]._delaySamples), static_cast<float>(_offsetSamples) }
				: Shaper{ _lastPointValue, 0, 1, 0 };
		}

		template <typename Shaper>
		auto currentValue() const noexcept
		{
			return _nextIndex < _size
				? Shaper::value(_lastPointValue, _points[_nextIndex]._value - _lastPointValue, static_cast<float>(_points[_nextIndex]._delaySamples), static_cast<float>(_offsetSamples))
				: _lastPointValue;
		}

		constexpr auto maxContinuousAdvance() const noexcept
		{
			return _nextIndex < _size ? _points[_nextIndex]._delaySamples - _offsetSamples : std::numeric_limits<unsigned>::max();
		}

		template <typename Shaper>
		void start(bool fromCurrent) noexcept
		{
			_lastPointValue = fromCurrent ? currentValue<Shaper>() : 0;
			_nextIndex = 0;
			while (_nextIndex < _size && !_points[_nextIndex]._delaySamples)
				_lastPointValue = _points[_nextIndex++]._value;
			_offsetSamples = 0;
		}

		constexpr void stop() noexcept
		{
			_nextIndex = _size;
			_lastPointValue = _size > 0 ? _points[_size - 1]._value : 0;
			_offsetSamples = 0;
		}

		constexpr bool stopped() const noexcept
		{
			return _nextIndex == _size;
		}

		auto totalSamples() const noexcept
		{
			// std::accumulate is not constexpr in MSVC 2019 (16.5.4).
			return std::accumulate(_points, _points + _size, size_t{}, [](size_t result, const SampledPoint& point) { return result + point._delaySamples; });
		}

	private:
		const SampledPoint* const _points;
		const unsigned _size;
		float _lastPointValue{ _size > 0 ? _points[_size - 1]._value : 0 };
		unsigned _nextIndex = _size;
		unsigned _offsetSamples = 0;
	};
}
