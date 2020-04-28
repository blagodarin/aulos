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
#include <limits>
#include <numeric>

namespace aulos
{
	struct SampledEnvelope
	{
	public:
		struct Point
		{
			unsigned _delay;
			float _value;
		};

		SampledEnvelope(const Envelope&, unsigned samplingRate) noexcept;

		constexpr auto begin() const noexcept { return _points.data(); }
		constexpr auto end() const noexcept { return _end; }
		auto totalSamples() const noexcept { return std::accumulate(begin(), end(), size_t{}, [](size_t delay, const Point& point) { return delay + point._delay; }); }

	private:
		std::array<Point, 5> _points{};
		const Point* _end = _points.data();
	};

	class LinearModulator
	{
	public:
		constexpr LinearModulator(const SampledEnvelope& envelope) noexcept
			: _envelope{ envelope } {}

		void advance(unsigned samples) noexcept;
		constexpr auto maxContinuousAdvance() const noexcept { return _nextPoint != _envelope.end() ? _nextPoint->_delay - _offset : std::numeric_limits<unsigned>::max(); }
		void start(bool fromCurrent) noexcept;
		constexpr void stop() noexcept { _nextPoint = _envelope.end(); }
		constexpr bool stopped() const noexcept { return _nextPoint == _envelope.end(); }
		auto totalSamples() const noexcept { return _envelope.totalSamples(); }
		constexpr auto value() const noexcept { return _currentValue; }
		constexpr auto valueStep() const noexcept { return _step; }

	private:
		const SampledEnvelope& _envelope;
		const SampledEnvelope::Point* _nextPoint = _envelope.end();
		float _baseValue = 1.f;
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
