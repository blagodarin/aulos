// This file is part of the Aulos toolkit.
// Copyright (C) Sergei Blagodarin.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <aulos/common.hpp>
#include "modulator.hpp"
#include "oscillator.hpp"
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
			, _rectangularityOffset{ _asymmetryOffset + _asymmetrySize + 1 }
			, _rectangularitySize{ 1 + static_cast<unsigned>(data._rectangularityEnvelope._changes.size()) }
			, _tremolo{ data._tremolo }
			, _vibrato{ data._vibrato }
			, _asymmetryOscillation{ data._asymmetryOscillation }
			, _rectangularityOscillation{ data._rectangularityOscillation }
		{
			_pointBuffer.reserve(size_t{ _rectangularityOffset } + _rectangularitySize + 1);
			addPoints<Transformation::None>(data._amplitudeEnvelope, samplingRate);
			addPoints<Transformation::Exp2>(data._frequencyEnvelope, samplingRate);
			addPoints<Transformation::None>(data._asymmetryEnvelope, samplingRate);
			addPoints<Transformation::None>(data._rectangularityEnvelope, samplingRate);
		}

		[[nodiscard]] std::span<const SampledPoint> amplitudePoints() const noexcept { return { _pointBuffer.data(), _amplitudeSize }; }
		[[nodiscard]] constexpr auto& asymmetryOscillation() const noexcept { return _asymmetryOscillation; }
		[[nodiscard]] std::span<const SampledPoint> asymmetryPoints() const noexcept { return { _pointBuffer.data() + _asymmetryOffset, _asymmetrySize }; }
		[[nodiscard]] std::span<const SampledPoint> frequencyPoints() const noexcept { return { _pointBuffer.data() + _frequencyOffset, _frequencySize }; }
		[[nodiscard]] constexpr auto& rectangularityOscillation() const noexcept { return _rectangularityOscillation; }
		[[nodiscard]] std::span<const SampledPoint> rectangularityPoints() const noexcept { return { _pointBuffer.data() + _rectangularityOffset, _rectangularitySize }; }
		[[nodiscard]] constexpr auto shapeParameter() const noexcept { return _shapeParameter; }
		[[nodiscard]] constexpr auto& tremolo() const noexcept { return _tremolo; }
		[[nodiscard]] constexpr auto& vibrato() const noexcept { return _vibrato; }

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
		const unsigned _rectangularityOffset;
		const unsigned _rectangularitySize;
		primal::RigidVector<SampledPoint> _pointBuffer;
		Oscillation _tremolo;
		Oscillation _vibrato;
		Oscillation _asymmetryOscillation;
		Oscillation _rectangularityOscillation;
	};

	class WaveState
	{
	public:
		WaveState(const WaveData& data, unsigned samplingRate) noexcept
			: _samplingRate{ static_cast<float>(samplingRate) }
			, _shapeParameter{ data.shapeParameter() }
			, _amplitudeModulator{ data.amplitudePoints() }
			, _amplitudeOscillator{ data.tremolo()._frequency / _samplingRate, data.tremolo()._magnitude }
			, _frequencyModulator{ data.frequencyPoints() }
			, _frequencyOscillator{ data.vibrato()._frequency / _samplingRate, 1 - std::exp2(-data.vibrato()._magnitude) }
			, _asymmetryModulator{ data.asymmetryPoints() }
			, _asymmetryOscillator{ data.asymmetryOscillation()._frequency / _samplingRate, data.asymmetryOscillation()._magnitude }
			, _rectangularityModulator{ data.rectangularityPoints() }
			, _rectangularityOscillator{ data.rectangularityOscillation()._frequency / _samplingRate, data.rectangularityOscillation()._magnitude }
		{
		}

		void advance(int samples) noexcept
		{
			assert(samples > 0);
			if (!_period.stopped())
				_period.advance(static_cast<float>(samples));
			if (_needRestart)
				_restartDelay -= samples;
		}

		[[nodiscard]] int prepareAdvance() noexcept
		{
			if (_period.stopped())
			{
				if (_needRestart && _restartDelay <= 0)
				{
					_needRestart = false;
					startWave(_restartFrequency, _restartAmplitude, static_cast<float>(-_restartDelay));
				}
				else
				{
					if (_amplitudeModulator.stopped())
					{
						_period = {};
						return _needRestart ? _restartDelay : std::numeric_limits<int>::max();
					}
					_offset += _periodLength;
					startWavePeriod();
				}
			}
			return static_cast<int>(std::ceil(_period.maxAdvance()));
		}

		[[nodiscard]] constexpr ShaperData shaperData() const noexcept
		{
			return _period.shaperData(_periodRectangularity, _shapeParameter);
		}

		void start(float frequency, float amplitude, int delay) noexcept
		{
			assert(frequency > 0);
			assert(amplitude > 0);
			assert(delay >= 0);
			assert(!_needRestart); // TODO: Come up with a way to handle frequent wave restarts.
			if (_period.stopped() && delay == 0)
			{
				startWave(frequency, amplitude, 0);
			}
			else
			{
				_needRestart = true;
				_restartFrequency = frequency;
				_restartAmplitude = amplitude;
				_restartDelay = delay;
			}
		}

		constexpr void stop() noexcept
		{
			_amplitudeModulator.stop();
			_period = {};
			_needRestart = false;
		}

	private:
		[[nodiscard]] static constexpr float adjust(float value, float adjustment) noexcept
		{
			assert(value >= 0.f && value <= 1.f);
			assert(adjustment >= 0.f && adjustment <= 1.f);
			return value + (1 - value) * adjustment;
		}

		void startWave(float frequency, float amplitude, float offsetSamples) noexcept
		{
			assert(frequency > 0);
			assert(amplitude > 0);
			assert(offsetSamples >= 0);
			_amplitudeModulator.start(offsetSamples);
			_frequencyModulator.start(offsetSamples);
			_asymmetryModulator.start(offsetSamples);
			_rectangularityModulator.start(offsetSamples);
			_frequency = frequency;
			_amplitude = amplitude;
			_offset = offsetSamples;
			_periodLength = 0;
			startWavePeriod();
		}

		void startWavePeriod() noexcept
		{
			const auto periodFrequency = _frequency * _frequencyModulator.advance(_periodLength) * (1 - _frequencyOscillator.value(_offset));
			assert(periodFrequency > 0);
			_periodLength = _samplingRate / periodFrequency;
			const auto periodAmplitude = _amplitude * _amplitudeModulator.advance(_periodLength) * (1 - _amplitudeOscillator.value(_offset));
			const auto periodAsymmetry = adjust(_asymmetryModulator.advance(_periodLength), _asymmetryOscillator.value(_offset));
			_periodRectangularity = adjust(_rectangularityModulator.advance(_periodLength), _rectangularityOscillator.value(_offset));
			_period.start(_periodLength, periodAmplitude, periodAsymmetry, _amplitudeModulator.stopped());
		}

	private:
		const float _samplingRate;
		const float _shapeParameter;
		Modulator _amplitudeModulator;
		TriangleOscillator _amplitudeOscillator;
		Modulator _frequencyModulator;
		TriangleOscillator _frequencyOscillator;
		Modulator _asymmetryModulator;
		TriangleOscillator _asymmetryOscillator;
		Modulator _rectangularityModulator;
		TriangleOscillator _rectangularityOscillator;
		WavePeriod _period;
		float _offset = 0;
		float _periodLength = 0;
		float _periodRectangularity = 0;
		float _frequency = 0;
		float _amplitude = 0;
		bool _needRestart = false;
		int _restartDelay = 0;
		float _restartFrequency = 0;
		float _restartAmplitude = 0;
	};
}
