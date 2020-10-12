// This file is part of the Aulos toolkit.
// Copyright (C) Sergei Blagodarin.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "modulator.hpp"
#include "oscillator.hpp"
#include "tables.hpp"
#include "utils/limited_vector.hpp"

namespace aulos
{
	constexpr int stereoDelay(Note note, int offset, int radius) noexcept
	{
		constexpr int kLastNoteIndex = kNoteCount - 1;
		return offset + radius * (2 * static_cast<int>(note) - kLastNoteIndex) / kLastNoteIndex;
	}

	class WaveData
	{
	public:
		WaveData(const VoiceData& data, const TrackProperties& trackProperties, unsigned samplingRate, bool stereo)
			: _stereoOffset{ stereo ? static_cast<int>(std::lround(static_cast<float>(samplingRate) * trackProperties._stereoDelay / 1'000)) : 0 }
			, _stereoRadius{ stereo ? static_cast<int>(std::lround(static_cast<float>(samplingRate) * trackProperties._stereoRadius / 1'000)) : 0 }
			, _shapeParameter{ data._waveShapeParameter }
			, _amplitudeSize{ static_cast<unsigned>(data._amplitudeEnvelope._changes.size()) }
			, _frequencyOffset{ _amplitudeSize + 1 }
			, _frequencySize{ static_cast<unsigned>(data._frequencyEnvelope._changes.size()) }
			, _asymmetryOffset{ _frequencyOffset + _frequencySize + 1 }
			, _asymmetrySize{ static_cast<unsigned>(data._asymmetryEnvelope._changes.size()) }
			, _oscillationOffset{ _asymmetryOffset + _asymmetrySize + 1 }
			, _oscillationSize{ static_cast<unsigned>(data._oscillationEnvelope._changes.size()) }
		{
			const auto addPoints = [this](const aulos::Envelope& envelope, unsigned samplingRate) {
				for (const auto& change : envelope._changes)
					_pointBuffer.emplace_back(static_cast<unsigned>(change._duration.count() * samplingRate / 1000), change._value);
				_pointBuffer.emplace_back(std::numeric_limits<unsigned>::max(), envelope._changes.empty() ? 0 : _pointBuffer.back()._value);
			};

			_pointBuffer.reserve(_oscillationOffset + _oscillationSize + 1);
			addPoints(data._amplitudeEnvelope, samplingRate);
			addPoints(data._frequencyEnvelope, samplingRate);
			addPoints(data._asymmetryEnvelope, samplingRate);
			addPoints(data._oscillationEnvelope, samplingRate);
			_soundSamples = std::accumulate(_pointBuffer.begin(), _pointBuffer.begin() + _amplitudeSize, 0u, [](unsigned result, const SampledPoint& point) { return result + point._delaySamples; });
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

		constexpr auto stereoOffset() const noexcept
		{
			return _stereoOffset;
		}

		constexpr auto stereoRadius() const noexcept
		{
			return _stereoRadius;
		}

		auto totalSamples(Note note) const noexcept
		{
			return _soundSamples + std::abs(stereoDelay(note, _stereoOffset, _stereoRadius));
		}

	private:
		const int _stereoOffset;
		const int _stereoRadius;
		const float _shapeParameter;
		const unsigned _amplitudeSize;
		const unsigned _frequencyOffset;
		const unsigned _frequencySize;
		const unsigned _asymmetryOffset;
		const unsigned _asymmetrySize;
		const unsigned _oscillationOffset;
		const unsigned _oscillationSize;
		LimitedVector<SampledPoint> _pointBuffer;
		unsigned _soundSamples;
	};

	class WaveState
	{
	public:
		WaveState(const WaveData& data, unsigned samplingRate) noexcept
			: _shapeParameter{ data.shapeParameter() }
			, _amplitudeModulator{ data.amplitudePoints() }
			, _frequencyModulator{ data.frequencyPoints() }
			, _asymmetryModulator{ data.asymmetryPoints() }
			, _oscillationModulator{ data.oscillationPoints() }
			, _oscillator{ samplingRate }
		{
		}

		unsigned advance(unsigned maxSamples) noexcept
		{
			assert(maxSamples > 0);
			if (_startDelay > 0)
			{
				assert(!_restartDelay);
				if (_startDelay < maxSamples)
					return std::exchange(_startDelay, 0u);
				_startDelay -= maxSamples;
				return maxSamples;
			}
			auto samples = std::min(_amplitudeModulator.maxContinuousAdvance(), _oscillator.maxAdvance());
			if (_restartDelay > 0 && samples > _restartDelay)
				samples = _restartDelay;
			if (samples > maxSamples)
				samples = maxSamples;
			assert(_frequency > 0);
			_amplitudeModulator.advance(samples);
			_frequencyModulator.advance(samples);
			_asymmetryModulator.advance(samples);
			_oscillationModulator.advance(samples);
			_oscillator.advance(static_cast<float>(samples), _frequency * std::pow(2.f, _frequencyModulator.currentValue<LinearShaper>()), _asymmetryModulator.currentValue<LinearShaper>());
			if (_restartDelay > 0)
			{
				assert(!_startDelay);
				assert(_restartDelay >= samples);
				_restartDelay -= samples;
				if (!_restartDelay)
					startWave(_restartFrequency, true);
			}
			return samples;
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

		ShaperData waveShaperData(float amplitude) const noexcept
		{
			const auto orientedAmplitude = amplitude * _oscillator.stageSign();
			return { orientedAmplitude, -2 * orientedAmplitude * (1 - _oscillationModulator.currentValue<LinearShaper>()), _oscillator.stageLength(), _shapeParameter, _oscillator.stageOffset() };
		}

		void start(Note note, unsigned delay = 0) noexcept
		{
			const auto frequency = kNoteFrequencies[note];
			assert(frequency > 0);
			assert(!_restartDelay);
			if (_amplitudeModulator.stopped() || _startDelay > 0)
			{
				startWave(frequency, false);
				_startDelay = delay;
			}
			else if (!delay)
			{
				startWave(frequency, true);
				_startDelay = delay;
			}
			else
			{
				_restartDelay = delay;
				_restartFrequency = frequency;
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
		void startWave(float frequency, bool fromCurrent) noexcept
		{
			_amplitudeModulator.start(fromCurrent ? _amplitudeModulator.currentValue<LinearShaper>() : 0);
			_frequencyModulator.start(0);
			_asymmetryModulator.start(0);
			_oscillationModulator.start(0);
			_oscillator.start(frequency * std::pow(2.f, _frequencyModulator.currentBaseValue()), _asymmetryModulator.currentBaseValue(), fromCurrent);
			_frequency = frequency;
		}

	private:
		const float _shapeParameter;
		Modulator _amplitudeModulator;
		Modulator _frequencyModulator;
		Modulator _asymmetryModulator;
		Modulator _oscillationModulator;
		Oscillator _oscillator;
		float _frequency = 0;
		unsigned _startDelay = 0;
		unsigned _restartDelay = 0;
		float _restartFrequency = 0;
	};
}
