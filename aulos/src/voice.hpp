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

#include <cassert>
#include <cmath>

namespace aulos
{
	// A wave period is represented by two stages.
	// The first stage (+1) starts at maximum amplitude and advances towards the minimum,
	// and the second stage (-1) starts at minimum amplitude and advances towards the maximum.
	// Oscillator holds the state of a current stage and controls switching between stages.
	class Oscillator
	{
	public:
		constexpr explicit Oscillator(unsigned samplingRate) noexcept
			: _samplingRate{ samplingRate } {}

		void adjust(float frequency, float asymmetry) noexcept;
		void advance(unsigned samples, float nextFrequency, float nextAsymmetry) noexcept;
		auto maxAdvance() const noexcept { return static_cast<unsigned>(std::ceil(_stageRemainder)); }
		void restart(float frequency, float asymmetry) noexcept;
		constexpr auto samplingRate() const noexcept { return _samplingRate; }
		constexpr auto stageLength() const noexcept { return _stageLength; }
		constexpr auto stageOffset() const noexcept { return _stageLength - _stageRemainder; }
		constexpr auto stageSign() const noexcept { return _amplitudeSign; }

	private:
		void resetStage(float frequency, float asymmetry) noexcept;

	private:
		const unsigned _samplingRate;
		float _stageLength = 0.f;
		float _stageRemainder = 0.f;
		float _amplitudeSign = 1.f;
	};

	class WaveState
	{
	public:
		WaveState(const ModulationData& modulation, unsigned samplingRate, float delay) noexcept
			: _modulator{ modulation }
			, _oscillator{ samplingRate }
			, _delay{ static_cast<size_t>(std::lround(samplingRate * delay / 1'000.f)) }
		{
			assert(delay >= 0.f);
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
			_modulator.advance(samples);
			_oscillator.advance(samples, _frequency * _modulator.currentFrequency(), _modulator.currentAsymmetry());
			if (_restartDelay > 0)
			{
				assert(!_startDelay);
				assert(_restartDelay >= samples);
				_restartDelay -= samples;
				if (!_restartDelay)
				{
					_modulator.start();
					_oscillator.adjust(_restartFrequency * _modulator.currentFrequency(), _modulator.currentAsymmetry());
					_frequency = _restartFrequency;
				}
			}
		}

		template <typename Generator>
		auto createGenerator(float amplitude) const noexcept
		{
			const auto orientedAmplitude = amplitude * _oscillator.stageSign();
			return Generator{ _oscillator.stageLength(), _oscillator.stageOffset(), orientedAmplitude, _modulator.currentOscillation() * orientedAmplitude };
		}

		auto linearChange() const noexcept
		{
			return std::pair{ _modulator.currentAmplitude(), _startDelay ? 0.f : _modulator.currentAmplitudeStep() };
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
				return std::min({ _modulator.maxAdvance(), _oscillator.maxAdvance(), _restartDelay });
			}
			return std::min(_modulator.maxAdvance(), _oscillator.maxAdvance());
		}

		void restart() noexcept
		{
			assert(_frequency > 0);
			_modulator.stop();
			_modulator.start();
			_oscillator.restart(_frequency * _modulator.currentFrequency(), _modulator.currentAsymmetry());
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
			const auto frequency = noteFrequency(note);
			assert(frequency > 0);
			assert(!_restartDelay);
			if (_modulator.stopped() || _startDelay > 0)
			{
				_modulator.start();
				_oscillator.restart(frequency * _modulator.currentFrequency(), _modulator.currentAsymmetry());
				_frequency = frequency;
				_startDelay = _delay;
			}
			else if (!_delay)
			{
				_modulator.start();
				_oscillator.adjust(frequency * _modulator.currentFrequency(), _modulator.currentAsymmetry());
				_frequency = frequency;
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
			_modulator.stop();
		}

		bool stopped() const noexcept
		{
			return _modulator.stopped();
		}

		size_t totalSamples() const noexcept
		{
			return _delay + _modulator.totalSamples();
		}

	private:
		static float noteFrequency(Note) noexcept;

	private:
		Modulator _modulator;
		Oscillator _oscillator;
		const unsigned _delay;
		float _frequency = 0.;
		unsigned _startDelay = 0;
		unsigned _restartDelay = 0;
		float _restartFrequency = 0.;
	};

	class VoiceImpl : public VoiceRenderer
	{
	public:
		VoiceImpl(const VoiceData& data, unsigned samplingRate) noexcept
			: _modulationData{ data, samplingRate } {}

		virtual void stop() noexcept = 0;

	protected:
		const ModulationData _modulationData;
		float _baseAmplitude = 0.f;
	};

	template <typename Generator>
	class MonoVoice final : public VoiceImpl
	{
	public:
		static constexpr auto kBlockSize = sizeof(float);

		MonoVoice(const VoiceData& data, unsigned samplingRate) noexcept
			: VoiceImpl{ data, samplingRate }
			, _wave{ _modulationData, samplingRate, 0.f }
		{
		}

		unsigned channels() const noexcept override
		{
			return 1;
		}

		size_t render(void* buffer, size_t bufferBytes) noexcept override
		{
			bufferBytes -= bufferBytes % kBlockSize;
			size_t offset = 0;
			while (offset < bufferBytes && !_wave.stopped())
			{
				const auto blocksToGenerate = static_cast<unsigned>(std::min<size_t>((bufferBytes - offset) / kBlockSize, _wave.maxAdvance()));
				if (buffer)
				{
					const auto output = reinterpret_cast<float*>(static_cast<std::byte*>(buffer) + offset);
					auto generator = _wave.createGenerator<Generator>(_baseAmplitude);
					auto [multiplier, step] = _wave.linearChange();
					for (size_t i = 0; i < blocksToGenerate; ++i)
					{
						output[i] += generator() * multiplier;
						multiplier += step;
					}
				}
				_wave.advance(blocksToGenerate);
				offset += blocksToGenerate * kBlockSize;
			}
			return offset;
		}

		void restart() noexcept override
		{
			_wave.restart();
		}

		unsigned samplingRate() const noexcept override
		{
			return _wave.samplingRate();
		}

		void start(Note note, float amplitude) noexcept override
		{
			_baseAmplitude = std::clamp(amplitude, -1.f, 1.f);
			_wave.start(note);
		}

		void stop() noexcept override
		{
			_wave.stop();
		}

		size_t totalSamples() const noexcept override
		{
			return _wave.totalSamples();
		}

	private:
		WaveState _wave;
	};

	class BasicStereoVoice : public VoiceImpl
	{
	public:
		static constexpr auto kBlockSize = 2 * sizeof(float);

		BasicStereoVoice(const VoiceData& data, unsigned samplingRate) noexcept
			: VoiceImpl{ data, samplingRate }
			, _leftAmplitude{ std::min(1.f - data._pan, 1.f) }
			, _rightAmplitude{ std::copysign(std::min(1.f + data._pan, 1.f), data._antiphase ? -1.f : 1.f) }
		{
		}

		unsigned channels() const noexcept final
		{
			return 2;
		}

	protected:
		const float _leftAmplitude;
		const float _rightAmplitude;
	};

	template <typename Generator>
	class StereoVoice final : public BasicStereoVoice
	{
	public:
		StereoVoice(const VoiceData& data, unsigned samplingRate) noexcept
			: BasicStereoVoice{ data, samplingRate }
			, _wave{ _modulationData, samplingRate, 0.f }
		{
		}

		size_t render(void* buffer, size_t bufferBytes) noexcept override
		{
			bufferBytes -= bufferBytes % kBlockSize;
			size_t offset = 0;
			while (offset < bufferBytes && !_wave.stopped())
			{
				const auto blocksToGenerate = static_cast<unsigned>(std::min<size_t>((bufferBytes - offset) / kBlockSize, _wave.maxAdvance()));
				if (buffer)
				{
					auto output = reinterpret_cast<float*>(static_cast<std::byte*>(buffer) + offset);
					auto generator = _wave.createGenerator<Generator>(_baseAmplitude);
					auto [multiplier, step] = _wave.linearChange();
					for (size_t i = 0; i < blocksToGenerate; ++i)
					{
						const auto value = generator() * multiplier;
						*output++ += value * _leftAmplitude;
						*output++ += value * _rightAmplitude;
						multiplier += step;
					}
				}
				_wave.advance(blocksToGenerate);
				offset += blocksToGenerate * kBlockSize;
			}
			return offset;
		}

		void restart() noexcept override
		{
			_wave.restart();
		}

		unsigned samplingRate() const noexcept override
		{
			return _wave.samplingRate();
		}

		void start(Note note, float amplitude) noexcept override
		{
			_baseAmplitude = std::clamp(amplitude, -1.f, 1.f);
			_wave.start(note);
		}

		void stop() noexcept override
		{
			_wave.stop();
		}

		size_t totalSamples() const noexcept override
		{
			return _wave.totalSamples();
		}

	private:
		WaveState _wave;
	};

	template <typename Generator>
	class PhasedStereoVoice final : public BasicStereoVoice
	{
	public:
		PhasedStereoVoice(const VoiceData& data, unsigned samplingRate) noexcept
			: BasicStereoVoice{ data, samplingRate }
			, _leftWave{ _modulationData, samplingRate, std::max(0.f, -data._phaseShift) }
			, _rightWave{ _modulationData, samplingRate, std::max(0.f, data._phaseShift) }
		{
		}

		size_t render(void* buffer, size_t bufferBytes) noexcept override
		{
			bufferBytes -= bufferBytes % kBlockSize;
			size_t offset = 0;
			while (offset < bufferBytes && !(_leftWave.stopped() && _rightWave.stopped()))
			{
				const auto samplesToGenerate = static_cast<unsigned>(std::min<size_t>({ (bufferBytes - offset) / kBlockSize, _leftWave.maxAdvance(), _rightWave.maxAdvance() }));
				if (buffer)
				{
					auto output = reinterpret_cast<float*>(static_cast<std::byte*>(buffer) + offset);
					auto leftGenerator = _leftWave.createGenerator<Generator>(_baseAmplitude * _leftAmplitude);
					auto rightGenerator = _rightWave.createGenerator<Generator>(_baseAmplitude * _rightAmplitude);
					auto [leftMultiplier, leftStep] = _leftWave.linearChange();
					auto [rightMultiplier, rightStep] = _rightWave.linearChange();
					for (size_t i = 0; i < samplesToGenerate; ++i)
					{
						*output++ += leftGenerator() * leftMultiplier;
						*output++ += rightGenerator() * rightMultiplier;
						leftMultiplier += leftStep;
						rightMultiplier += rightStep;
					}
				}
				_leftWave.advance(samplesToGenerate);
				_rightWave.advance(samplesToGenerate);
				offset += samplesToGenerate * kBlockSize;
			}
			return offset;
		}

		void restart() noexcept override
		{
			_leftWave.restart();
			_rightWave.restart();
		}

		unsigned samplingRate() const noexcept override
		{
			return _leftWave.samplingRate();
		}

		void start(Note note, float amplitude) noexcept override
		{
			_baseAmplitude = std::clamp(amplitude, -1.f, 1.f);
			_leftWave.start(note);
			_rightWave.start(note);
		}

		void stop() noexcept override
		{
			_leftWave.stop();
			_rightWave.stop();
		}

		size_t totalSamples() const noexcept override
		{
			return std::max(_leftWave.totalSamples(), _rightWave.totalSamples());
		}

	private:
		WaveState _leftWave;
		WaveState _rightWave;
	};
}
