// This file is part of the Aulos toolkit.
// Copyright (C) Sergei Blagodarin.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <aulos/common.hpp>
#include "modulator.hpp"
#include "period.hpp"

#include <primal/rigid_vector.hpp>

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
		primal::RigidVector<SampledPoint> _pointBuffer;
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

		void advance(int samples) noexcept
		{
			assert(samples > 0);
			if (!_started)
			{
				if (_needRestart)
				{
					assert(_restartDelay > 0);
					assert(samples <= _restartDelay);
					_restartDelay -= samples;
					if (_restartDelay == 0)
					{
						_needRestart = false;
						startWave(_restartFrequency, _restartAmplitude);
					}
				}
				return;
			}
			assert(!_period.stopped());
			assert(samples <= static_cast<int>(std::ceil(_period.maxAdvance())));
			if (_needRestart)
				_restartDelay -= samples;
			if (_period.advance(static_cast<float>(samples)))
				return;
			assert(_period.stopped());
			if (_needRestart && _restartDelay <= 0)
			{
				_needRestart = false;
				startWave(_restartFrequency, _restartAmplitude);
				return;
			}
			if (_amplitudeModulator.stopped())
			{
				_period = {};
				_started = false;
				return;
			}
			const auto periodFrequency = _frequency * _frequencyModulator.advance(_periodLength);
			assert(periodFrequency > 0);
			_periodLength = _samplingRate / periodFrequency;
			const auto periodAmplitude = _amplitude * _amplitudeModulator.advance(_periodLength);
			const auto periodAsymmetry = _asymmetryModulator.advance(_periodLength);
			_period.start(_periodLength, periodAmplitude, periodAsymmetry);
			_periodOscillation = _oscillationModulator.advance(_periodLength);
		}

		int maxAdvance() const noexcept
		{
			if (_started)
				return static_cast<int>(std::ceil(_period.maxAdvance()));
			if (_needRestart)
			{
				assert(_restartDelay > 0);
				return _restartDelay;
			}
			assert(_period.stopped());
			return std::numeric_limits<int>::max();
		}

		ShaperData waveShaperData() const noexcept
		{
			return _period.currentShaperData(_periodOscillation, _shapeParameter);
		}

		void start(float frequency, float amplitude, int delay) noexcept
		{
			assert(frequency > 0);
			assert(amplitude > 0);
			assert(delay >= 0);
			if (!_started)
			{
				if (delay == 0)
				{
					startWave(frequency, amplitude);
					return;
				}
				else
					assert(!_needRestart); // TODO: Come up with a way to handle frequent wave restarts.
			}
			_needRestart = true;
			_restartFrequency = frequency;
			_restartAmplitude = amplitude;
			_restartDelay = delay;
		}

		constexpr void stop() noexcept
		{
			_started = false;
			_needRestart = false;
		}

	private:
		void startWave(float frequency, float amplitude) noexcept
		{
			assert(frequency > 0);
			_amplitudeModulator.start();
			_frequencyModulator.start();
			_asymmetryModulator.start();
			_oscillationModulator.start();
			_frequency = frequency;
			_amplitude = amplitude;
			const auto periodFrequency = _frequency * _frequencyModulator.currentValue();
			assert(periodFrequency > 0);
			_periodLength = _samplingRate / periodFrequency;
			const auto periodAmplitude = _amplitude * _amplitudeModulator.advance(_periodLength);
			const auto periodAsymmetry = _asymmetryModulator.advance(_periodLength);
			_period.start(_periodLength, periodAmplitude, periodAsymmetry);
			_periodOscillation = _oscillationModulator.advance(_periodLength);
			_started = true;
		}

	private:
		const float _samplingRate;
		const float _shapeParameter;
		Modulator _amplitudeModulator;
		Modulator _frequencyModulator;
		Modulator _asymmetryModulator;
		Modulator _oscillationModulator;
		WavePeriod _period;
		float _periodLength = 0;
		float _periodOscillation = 0;
		float _frequency = 0;
		float _amplitude = 0;
		bool _started = false;
		bool _needRestart = false;
		int _restartDelay = 0;
		float _restartFrequency = 0;
		float _restartAmplitude = 0;
	};
}
