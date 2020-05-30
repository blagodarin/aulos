// This file is part of the Aulos toolkit.
// Copyright (C) Sergei Blagodarin.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "modulator.hpp"
#include "note_table.hpp"
#include "oscillator.hpp"

#include <tuple>

namespace aulos
{
	class WaveData
	{
	public:
		WaveData(const VoiceData& data, unsigned samplingRate, bool stereo)
			: _absoluteDelay{ stereo ? static_cast<unsigned>(std::lround(samplingRate * std::abs(data._stereoDelay) / 1'000)) : 0 }
			, _shapeParameter{ data._waveShapeParameter }
		{
			std::tie(_amplitudeOffset, _amplitudeSize) = addPoints(data._amplitudeEnvelope, samplingRate);
			std::tie(_frequencyOffset, _frequencySize) = addPoints(data._frequencyEnvelope, samplingRate);
			std::tie(_asymmetryOffset, _asymmetrySize) = addPoints(data._asymmetryEnvelope, samplingRate);
			std::tie(_oscillationOffset, _oscillationSize) = addPoints(data._oscillationEnvelope, samplingRate);
		}

		constexpr auto absoluteDelay() const noexcept
		{
			return _absoluteDelay;
		}

		SampledPoints amplitudePoints() const noexcept
		{
			return { _pointBuffer.data() + _amplitudeOffset, _amplitudeSize };
		}

		SampledPoints asymmetryPoints() const noexcept
		{
			return { _pointBuffer.data() + _asymmetryOffset, _asymmetrySize };
		}

		SampledPoints frequencyPoints() const noexcept
		{
			return { _pointBuffer.data() + _frequencyOffset, _frequencySize };
		}

		SampledPoints oscillationPoints() const noexcept
		{
			return { _pointBuffer.data() + _oscillationOffset, _oscillationSize };
		}

		constexpr auto shapeParameter() const noexcept
		{
			return _shapeParameter;
		}

		auto totalSamples() const noexcept
		{
			const auto begin = _pointBuffer.data() + _amplitudeOffset;
			return _absoluteDelay + std::accumulate(begin, begin + _amplitudeSize, size_t{}, [](size_t result, const SampledPoint& point) { return result + point._delaySamples; });
		}

	private:
		std::pair<unsigned, unsigned> addPoints(const aulos::Envelope& envelope, unsigned samplingRate)
		{
			constexpr auto split2 = [](unsigned value) {
				const auto quotient = value / 2;
				const auto remainder = value % 2;
				return std::pair{ quotient, quotient + remainder };
			};

			constexpr auto split4 = [](unsigned value) {
				const auto quotient = value / 4;
				const auto remainder = value % 4;
				return std::tuple{
					quotient,
					quotient + static_cast<unsigned>(remainder > 2),
					quotient + static_cast<unsigned>(remainder > 1),
					quotient + static_cast<unsigned>(remainder > 0),
				};
			};

			const auto offset = _pointBuffer.size();
			auto lastValue = 0.f;
			for (const auto& change : envelope._changes)
			{
				const auto duration = static_cast<unsigned>(change._duration.count() * samplingRate / 1000);
				switch (change._shape)
				{
				case EnvelopeShape::Linear:
					_pointBuffer.emplace_back(duration, change._value);
					break;
				case EnvelopeShape::SmoothQuadratic2: {
					const auto [t0, t1] = split2(duration);
					_pointBuffer.emplace_back(t0, lastValue + (change._value - lastValue) / 4);
					_pointBuffer.emplace_back(t1, change._value);
					break;
				}
				case EnvelopeShape::SmoothQuadratic4: {
					const auto [t0, t1, t2, t3] = split4(duration);
					const auto delta = (change._value - lastValue) / 16;
					_pointBuffer.emplace_back(t0, lastValue + delta);
					_pointBuffer.emplace_back(t1, lastValue + delta * 4);
					_pointBuffer.emplace_back(t2, lastValue + delta * 9);
					_pointBuffer.emplace_back(t3, change._value);
					break;
				}
				case EnvelopeShape::SharpQuadratic2: {
					const auto [t0, t1] = split2(duration);
					_pointBuffer.emplace_back(t0, lastValue + (change._value - lastValue) * .75f);
					_pointBuffer.emplace_back(t1, change._value);
					break;
				}
				case EnvelopeShape::SharpQuadratic4: {
					const auto [t0, t1, t2, t3] = split4(duration);
					const auto delta = (change._value - lastValue) / 16;
					_pointBuffer.emplace_back(t0, lastValue + delta * 5);
					_pointBuffer.emplace_back(t1, lastValue + delta * 12);
					_pointBuffer.emplace_back(t2, lastValue + delta * 15);
					_pointBuffer.emplace_back(t3, change._value);
					break;
				}
				}
				lastValue = change._value;
			}
			const auto size = _pointBuffer.size() - offset;
			_pointBuffer.emplace_back(std::numeric_limits<unsigned>::max(), envelope._changes.empty() ? 0 : _pointBuffer.back()._value);
			return { static_cast<unsigned>(offset), static_cast<unsigned>(size) };
		}

	private:
		const unsigned _absoluteDelay;
		const float _shapeParameter;
		std::vector<SampledPoint> _pointBuffer;
		unsigned _amplitudeOffset;
		unsigned _amplitudeSize;
		unsigned _frequencyOffset;
		unsigned _frequencySize;
		unsigned _asymmetryOffset;
		unsigned _asymmetrySize;
		unsigned _oscillationOffset;
		unsigned _oscillationSize;
	};

	class WaveState
	{
	public:
		WaveState(const WaveData& data, unsigned samplingRate, unsigned delay) noexcept
			: _shapeParameter{ data.shapeParameter() }
			, _amplitudeModulator{ data.amplitudePoints() }
			, _frequencyModulator{ data.frequencyPoints() }
			, _asymmetryModulator{ data.asymmetryPoints() }
			, _oscillationModulator{ data.oscillationPoints() }
			, _oscillator{ samplingRate }
			, _delay{ delay }
		{
		}

		void advance(unsigned samples) noexcept
		{
			assert(samples > 0);
			if (_startDelay > 0)
			{
				assert(_startDelay >= samples);
				assert(!_restartDelay);
				_startDelay -= samples;
				return;
			}
			assert(_frequency > 0);
			_amplitudeModulator.advance(samples);
			_frequencyModulator.advance(samples);
			_asymmetryModulator.advance(samples);
			_oscillationModulator.advance(samples);
			_oscillator.advance(samples, _frequency * std::pow(2.f, _frequencyModulator.currentValue<LinearShaper>()), _asymmetryModulator.currentValue<LinearShaper>());
			if (_restartDelay > 0)
			{
				assert(!_startDelay);
				assert(_restartDelay >= samples);
				_restartDelay -= samples;
				if (!_restartDelay)
					startWave(_restartFrequency, true);
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

		ShaperData waveShaperData(float amplitude) const noexcept
		{
			const auto orientedAmplitude = amplitude * _oscillator.stageSign();
			return { orientedAmplitude, -2 * orientedAmplitude * (1 - _oscillationModulator.currentValue<LinearShaper>()), _oscillator.stageLength(), _shapeParameter, _oscillator.stageOffset() };
		}

		auto maxAdvance() const noexcept
		{
			if (_startDelay > 0)
			{
				assert(!_restartDelay);
				return _startDelay;
			}
			if (_restartDelay > 0)
			{
				assert(!_startDelay);
				return std::min({ _amplitudeModulator.maxContinuousAdvance(), _oscillator.maxAdvance(), _restartDelay });
			}
			return std::min(_amplitudeModulator.maxContinuousAdvance(), _oscillator.maxAdvance());
		}

		void start(Note note) noexcept
		{
			const auto frequency = kNoteTable[note];
			assert(frequency > 0);
			assert(!_restartDelay);
			if (_amplitudeModulator.stopped() || _startDelay > 0)
			{
				startWave(frequency, false);
				_startDelay = _delay;
			}
			else if (!_delay)
			{
				startWave(frequency, true);
				_startDelay = _delay;
			}
			else
			{
				_restartDelay = _delay;
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
		const unsigned _delay;
		float _frequency = 0;
		unsigned _startDelay = 0;
		unsigned _restartDelay = 0;
		float _restartFrequency = 0;
	};
}
