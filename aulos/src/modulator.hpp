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

namespace aulos
{
	struct SampledEnvelope
	{
	public:
		struct Point
		{
			size_t _delay;
			double _value;
		};

		SampledEnvelope(const Envelope&, size_t samplingRate) noexcept;

		const Point* begin() const noexcept { return _points.data(); }
		size_t duration() const noexcept;
		const Point* end() const noexcept { return _points.data() + _size; }
		constexpr auto size() const noexcept { return _size; }

	private:
		size_t _size = 0;
		std::array<Point, 5> _points{};
	};

	class LinearModulator
	{
	public:
		LinearModulator(const SampledEnvelope&) noexcept;

		void advance(size_t samples) noexcept;
		size_t maxContinuousAdvance() const noexcept;
		void start(bool fromCurrent) noexcept;
		void stop() noexcept { _nextPoint = _envelope.end(); }
		bool stopped() const noexcept { return _nextPoint == _envelope.end(); }
		auto totalSamples() const noexcept { return _envelope.duration(); }
		constexpr double value() const noexcept { return _currentValue; }
		double valueStep() const noexcept;

	private:
		const SampledEnvelope& _envelope;
		double _baseValue = 1.0;
		const SampledEnvelope::Point* _nextPoint = _envelope.end();
		size_t _offset = 0;
		double _currentValue = 0.0;
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

		void advance(size_t samples) noexcept;
		constexpr auto currentAmplitude() const noexcept { return _amplitudeModulator.value(); }
		auto currentAmplitudeStep() const noexcept { return _amplitudeModulator.valueStep(); }
		constexpr auto currentAsymmetry() const noexcept { return _asymmetryModulator.value(); }
		constexpr auto currentFrequency() const noexcept { return _frequencyModulator.value(); }
		constexpr auto currentOscillation() const noexcept { return _oscillationModulator.value(); }
		auto maxAdvance() const noexcept { return _amplitudeModulator.maxContinuousAdvance(); }
		void start() noexcept;
		void stop() noexcept { _amplitudeModulator.stop(); }
		auto stopped() const noexcept { return _amplitudeModulator.stopped(); }
		auto totalSamples() const noexcept { return _amplitudeModulator.totalSamples(); }

	private:
		LinearModulator _amplitudeModulator;
		LinearModulator _frequencyModulator;
		LinearModulator _asymmetryModulator;
		LinearModulator _oscillationModulator;
	};
}
