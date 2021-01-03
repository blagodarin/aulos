// This file is part of the Aulos toolkit.
// Copyright (C) Sergei Blagodarin.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <aulos/common.hpp>
#include "modulator.hpp"
#include "period.hpp"
#include "utils/limited_vector.hpp"

namespace aulos
{
	enum class Transformation
	{
		None,
		Exp2,
	};

	template <Transformation>
	float transform(float) noexcept = delete;

	template <>
	constexpr float transform<Transformation::None>(float value) noexcept { return value; }

	template <>
	inline float transform<Transformation::Exp2>(float value) noexcept { return std::exp2(value); }

	class WaveData
	{
	public:
		WaveData(const VoiceData& data, unsigned samplingRate)
			: _shapeParameter{ data._waveShapeParameter }
			, _amplitudeSize{ 1 + static_cast<unsigned>(data._amplitudeEnvelope._changes.size()) }
			, _frequencyOffset{ _amplitudeSize + 1 }
			, _frequencySize{ 1 + static_cast<unsigned>(data._frequencyEnvelope._changes.size()) }
			, _asymmetryOffset{ _frequencyOffset + _frequencySize + 1 }
			, _asymmetrySize{ 1 + static_cast<unsigned>(data._asymmetryEnvelope._changes.size()) }
			, _oscillationOffset{ _asymmetryOffset + _asymmetrySize + 1 }
			, _oscillationSize{ 1 + static_cast<unsigned>(data._oscillationEnvelope._changes.size()) }
		{
			_pointBuffer.reserve(size_t{ _oscillationOffset } + _oscillationSize + 1);
			addPoints<Transformation::None>(data._amplitudeEnvelope, samplingRate);
			addPoints<Transformation::Exp2>(data._frequencyEnvelope, samplingRate);
			addPoints<Transformation::None>(data._asymmetryEnvelope, samplingRate);
			addPoints<Transformation::None>(data._oscillationEnvelope, samplingRate);
		}

		std::span<const SampledPoint> amplitudePoints() const noexcept
		{
			return { _pointBuffer.data(), _amplitudeSize };
		}

		std::span<const SampledPoint> asymmetryPoints() const noexcept
		{
			return { _pointBuffer.data() + _asymmetryOffset, _asymmetrySize };
		}

		std::span<const SampledPoint> frequencyPoints() const noexcept
		{
			return { _pointBuffer.data() + _frequencyOffset, _frequencySize };
		}

		std::span<const SampledPoint> oscillationPoints() const noexcept
		{
			return { _pointBuffer.data() + _oscillationOffset, _oscillationSize };
		}

		constexpr auto shapeParameter() const noexcept
		{
			return _shapeParameter;
		}

	private:
		template <Transformation transformation>
		void addPoints(const aulos::Envelope& envelope, unsigned samplingRate)
		{
			_pointBuffer.emplace_back(0u, transform<transformation>(0));
			for (const auto& change : envelope._changes)
				_pointBuffer.emplace_back(static_cast<unsigned>(change._duration.count() * samplingRate / 1000), transform<transformation>(change._value));
			_pointBuffer.emplace_back(std::numeric_limits<unsigned>::max(), _pointBuffer.back()._value);
		}

	private:
		const float _shapeParameter;
		const unsigned _amplitudeSize;
		const unsigned _frequencyOffset;
		const unsigned _frequencySize;
		const unsigned _asymmetryOffset;
		const unsigned _asymmetrySize;
		const unsigned _oscillationOffset;
		const unsigned _oscillationSize;
		LimitedVector<SampledPoint> _pointBuffer;
	};

	class WaveState
	{
	public:
		WaveState(const WaveData& data, unsigned samplingRate) noexcept
			: _samplingRate{ static_cast<float>(samplingRate) }
			, _shapeParameter{ data.shapeParameter() }
			, _amplitudeModulator{ data.amplitudePoints() }
			, _frequencyModulator{ data.frequencyPoints() }
			, _asymmetryModulator{ data.asymmetryPoints() }
			, _oscillationModulator{ data.oscillationPoints() }
		{
		}

		void advance(unsigned samples) noexcept
		{
			assert(samples > 0 && samples <= maxAdvance());
			if (_startDelay > 0)
			{
				assert(!_restartDelay);
				assert(samples <= _startDelay);
				_startDelay -= samples;
				return;
			}
			assert(!_restartDelay || samples <= _restartDelay);
			if (!_amplitudeModulator.stopped()) // TODO: Run benchmark with and without this check.
			{
				_amplitudeModulator.advance(samples);
				_frequencyModulator.advance(samples);
				_asymmetryModulator.advance(samples);
				_oscillationModulator.advance(samples);
				if (!_period.advance(static_cast<float>(samples)))
				{
					const auto nextFrequency = _frequency * _frequencyModulator.currentValue<LinearShaper>();
					assert(nextFrequency > 0);
					_period.restart(_samplingRate / nextFrequency, _asymmetryModulator.currentValue<LinearShaper>());
				}
			}
			if (_restartDelay > 0)
			{
				assert(!_startDelay);
				_restartDelay -= samples;
				if (!_restartDelay)
					startWave(_restartFrequency, _restartAmplitude, true);
			}
		}

		constexpr ShaperData amplitudeShaperData() const noexcept
		{
			// Moving the condition inside the modulator is a bit slower.
			// Also MSVC 16.5.4 fails to optimize ternary operators in return statements, so they're slower A LOT.
			if (!_startDelay)
				return _amplitudeModulator.shaperData();
			assert(!_amplitudeModulator.currentOffset());
			return { _amplitudeModulator.currentBaseValue() };
		}

		unsigned maxAdvance() const noexcept
		{
			if (_startDelay > 0)
				return _startDelay;
			const auto maxWaveAdvance = _amplitudeModulator.stopped()
				? std::numeric_limits<unsigned>::max()
				: std::min(_amplitudeModulator.maxContinuousAdvance(), _period.maxAdvance());
			if (_restartDelay > 0)
				return std::min(maxWaveAdvance, _restartDelay);
			return maxWaveAdvance;
		}

		ShaperData waveShaperData() const noexcept
		{
			return _period.currentShaperData(_oscillationModulator.currentValue<LinearShaper>(), _shapeParameter);
		}

		void start(float frequency, float amplitude, unsigned delay) noexcept
		{
			assert(!_restartDelay);
			if (_amplitudeModulator.stopped() || _startDelay > 0)
			{
				startWave(frequency, amplitude, false);
				_startDelay = delay;
			}
			else if (!delay)
			{
				startWave(frequency, amplitude, true);
				_startDelay = delay;
			}
			else
			{
				_restartDelay = delay;
				_restartFrequency = frequency;
				_restartAmplitude = amplitude;
			}
		}

		constexpr void stop() noexcept
		{
			_amplitudeModulator.stop();
		}

		constexpr bool stopped() const noexcept
		{
			return _amplitudeModulator.stopped();
		}

	private:
		void startWave(float frequency, float amplitude, bool fromCurrent) noexcept
		{
			assert(frequency > 0);
			_amplitudeModulator.start(fromCurrent ? std::optional{ _amplitudeModulator.currentValue<LinearShaper>() } : std::nullopt);
			_frequencyModulator.start({});
			_asymmetryModulator.start({});
			_oscillationModulator.start({});
			_frequency = frequency;
			const auto nextFrequency = _frequency * _frequencyModulator.currentBaseValue();
			assert(nextFrequency > 0);
			_period.start(_samplingRate / nextFrequency, amplitude, _asymmetryModulator.currentBaseValue(), fromCurrent);
		}

	private:
		const float _samplingRate;
		const float _shapeParameter;
		Modulator _amplitudeModulator;
		Modulator _frequencyModulator;
		Modulator _asymmetryModulator;
		Modulator _oscillationModulator;
		WavePeriod _period;
		float _frequency = 0;
		unsigned _startDelay = 0;
		unsigned _restartDelay = 0;
		float _restartFrequency = 0;
		float _restartAmplitude = 0;
	};
}
