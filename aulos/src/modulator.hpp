// This file is part of the Aulos toolkit.
// Copyright (C) Sergei Blagodarin.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "shaper.hpp"

#include <array>
#include <cassert>
#include <limits>
#include <numeric>
#include <span>

namespace aulos
{
	struct SampledPoint
	{
		unsigned _delaySamples;
		float _value;

		constexpr SampledPoint(unsigned delaySamples, float value) noexcept
			: _delaySamples{ delaySamples }, _value{ value } {}
	};

	class Modulator
	{
	public:
		explicit constexpr Modulator(std::span<const SampledPoint> points) noexcept
			: _points{ points.data() }
			, _size{ static_cast<unsigned>(points.size()) }
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

	private:
		const SampledPoint* const _points;
		const unsigned _size;
		float _lastPointValue{ _points[_size]._value };
		unsigned _nextIndex = _size;
		unsigned _offsetSamples = 0;
	};
}
