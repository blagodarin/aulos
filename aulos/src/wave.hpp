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

#include "modulator.hpp"
#include "note_table.hpp"
#include "oscillator.hpp"

namespace aulos
{
	struct WaveModulation
	{
		SampledEnvelope _amplitudeEnvelope;
		SampledEnvelope _frequencyEnvelope;
		SampledEnvelope _asymmetryEnvelope;
		SampledEnvelope _oscillationEnvelope;

		WaveModulation(const VoiceData& data, unsigned samplingRate) noexcept
			: _amplitudeEnvelope{ data._amplitudeEnvelope, samplingRate }
			, _frequencyEnvelope{ data._frequencyEnvelope, samplingRate }
			, _asymmetryEnvelope{ data._asymmetryEnvelope, samplingRate }
			, _oscillationEnvelope{ data._oscillationEnvelope, samplingRate }
		{
		}
	};

	class WaveState
	{
	public:
		WaveState(const WaveModulation& modulation, unsigned samplingRate, float delay) noexcept
			: _amplitudeModulator{ modulation._amplitudeEnvelope }
			, _frequencyModulator{ modulation._frequencyEnvelope }
			, _asymmetryModulator{ modulation._asymmetryEnvelope }
			, _oscillationModulator{ modulation._oscillationEnvelope }
			, _oscillator{ samplingRate }
			, _delay{ static_cast<unsigned>(std::lround(samplingRate * delay / 1'000)) }
		{
			assert(delay >= 0);
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
			_oscillator.advance(samples, _frequency * _frequencyModulator.value(), _asymmetryModulator.value());
			if (_restartDelay > 0)
			{
				assert(!_startDelay);
				assert(_restartDelay >= samples);
				_restartDelay -= samples;
				if (!_restartDelay)
					startWave(_restartFrequency, true);
			}
		}

		template <typename Generator>
		auto createGenerator(float amplitude) const noexcept
		{
			const auto orientedAmplitude = amplitude * _oscillator.stageSign();
			return Generator{ orientedAmplitude, -2 * orientedAmplitude * _oscillationModulator.value(), _oscillator.stageLength(), _oscillator.stageOffset() };
		}

		auto linearChange() const noexcept
		{
			return std::pair{ _amplitudeModulator.value(), _startDelay ? 0 : _amplitudeModulator.valueStep() };
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

		void restart() noexcept
		{
			assert(_frequency > 0);
			_amplitudeModulator.stop();
			startWave(_frequency, false);
			_startDelay = _delay;
			_restartDelay = 0;
			_restartFrequency = 0.;
		}

		auto samplingRate() const noexcept
		{
			return _oscillator.samplingRate();
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

		void stop() noexcept
		{
			_amplitudeModulator.stop();
		}

		bool stopped() const noexcept
		{
			return _amplitudeModulator.stopped();
		}

		size_t totalSamples() const noexcept
		{
			return _delay + _amplitudeModulator.totalSamples();
		}

	private:
		void startWave(float frequency, bool fromCurrent) noexcept
		{
			_amplitudeModulator.start(fromCurrent);
			_frequencyModulator.start(false);
			_asymmetryModulator.start(false);
			_oscillationModulator.start(false);
			_oscillator.start(frequency * _frequencyModulator.value(), _asymmetryModulator.value(), fromCurrent);
			_frequency = frequency;
		}

	private:
		LinearModulator _amplitudeModulator;
		LinearModulator _frequencyModulator;
		LinearModulator _asymmetryModulator;
		LinearModulator _oscillationModulator;
		Oscillator _oscillator;
		const unsigned _delay;
		float _frequency = 0;
		unsigned _startDelay = 0;
		unsigned _restartDelay = 0;
		float _restartFrequency = 0;
	};
}
