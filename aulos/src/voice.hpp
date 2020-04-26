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

		void adjustStage(double frequency, double asymmetry) noexcept;
		void advance(size_t samples, double nextFrequency, double nextAsymmetry) noexcept;
		auto maxAdvance() const noexcept { return static_cast<size_t>(std::ceil(_stageRemainder)); }
		void restart(double frequency, double asymmetry, double shift) noexcept;
		constexpr auto samplingRate() const noexcept { return _samplingRate; }
		constexpr auto stageLength() const noexcept { return _stageLength; }
		constexpr auto stageOffset() const noexcept { return _stageLength - _stageRemainder; }
		constexpr auto stageSign() const noexcept { return _amplitudeSign; }

	private:
		void resetStage(double frequency, double asymmetry) noexcept;

	private:
		const unsigned _samplingRate;
		double _stageLength = 0.0;
		double _stageRemainder = 0.0;
		float _amplitudeSign = 1.f;
	};

	class VoiceImpl : public VoiceRenderer
	{
	public:
		VoiceImpl(const VoiceData& data, unsigned samplingRate) noexcept
			: _modulationData{ data, samplingRate } {}

		virtual void stop() noexcept = 0;
		size_t totalSamples() const noexcept final { return _modulationData._amplitudeEnvelope.duration(); }

	protected:
		void startImpl(Note note, float amplitude) noexcept;

	protected:
		const ModulationData _modulationData;
		float _baseAmplitude = 0.f;
		double _baseFrequency = 0.0;
	};

	struct WaveState
	{
		Modulator _modulator;
		Oscillator _oscillator;

		WaveState(const ModulationData& modulation, unsigned samplingRate) noexcept
			: _modulator{ modulation }
			, _oscillator{ samplingRate }
		{
		}

		void advance(size_t samples, double frequency) noexcept
		{
			assert(samples > 0);
			assert(frequency > 0);
			_modulator.advance(samples);
			_oscillator.advance(samples, frequency * _modulator.currentFrequency(), _modulator.currentAsymmetry());
		}

		auto maxAdvance() const noexcept
		{
			return std::min(_modulator.maxAdvance(), _oscillator.maxAdvance());
		}

		void restart(double frequency, double delay) noexcept
		{
			assert(frequency > 0);
			assert(delay >= 0);
			_modulator.stop();
			_modulator.start();
			_oscillator.restart(frequency * _modulator.currentFrequency(), _modulator.currentAsymmetry(), delay);
		}

		void start(double frequency, double delay) noexcept
		{
			assert(frequency > 0);
			assert(delay >= 0);
			const bool wasStopped = _modulator.stopped();
			_modulator.start();
			const auto currentFrequency = frequency * _modulator.currentFrequency();
			const auto currentAsymmetry = _modulator.currentAsymmetry();
			if (wasStopped)
				_oscillator.restart(currentFrequency, currentAsymmetry, delay);
			else
				_oscillator.adjustStage(currentFrequency, currentAsymmetry);
		}

		void stop() noexcept
		{
			_modulator.stop();
		}

		bool stopped() const noexcept
		{
			return _modulator.stopped();
		}
	};

	// NOTE!
	// Keeping intermediate calculations in 'double' produces a measurable performance hit (e.g. 104ms -> 114 ms).

	template <typename Generator>
	class MonoVoice final : public VoiceImpl
	{
	public:
		static constexpr auto kBlockSize = sizeof(float);

		MonoVoice(const VoiceData& data, unsigned samplingRate) noexcept
			: VoiceImpl{ data, samplingRate }
			, _wave{ _modulationData, samplingRate }
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
				const auto blocksToGenerate = std::min((bufferBytes - offset) / kBlockSize, _wave.maxAdvance());
				if (buffer)
				{
					const auto amplitude = _baseAmplitude * _wave._oscillator.stageSign();
					Generator generator{ _wave._oscillator.stageLength(), _wave._oscillator.stageOffset(), amplitude, _wave._modulator.currentOscillation() * amplitude };
					const auto base = reinterpret_cast<float*>(static_cast<std::byte*>(buffer) + offset);
					auto amplitudeModulation = _wave._modulator.currentAmplitude();
					const auto step = _wave._modulator.currentAmplitudeStep();
					for (size_t i = 0; i < blocksToGenerate; ++i)
					{
						base[i] += static_cast<float>(generator() * amplitudeModulation);
						amplitudeModulation += step;
					}
				}
				_wave.advance(blocksToGenerate, _baseFrequency);
				offset += blocksToGenerate * kBlockSize;
			}
			return offset;
		}

		void restart() noexcept override
		{
			_wave.restart(_baseFrequency, 0);
		}

		unsigned samplingRate() const noexcept override
		{
			return _wave._oscillator.samplingRate();
		}

		void start(Note note, float amplitude) noexcept override
		{
			startImpl(note, amplitude);
			_wave.start(_baseFrequency, 0);
		}

		void stop() noexcept override
		{
			_wave.stop();
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
			, _wave{ _modulationData, samplingRate }
		{
		}

		size_t render(void* buffer, size_t bufferBytes) noexcept override
		{
			bufferBytes -= bufferBytes % kBlockSize;
			size_t offset = 0;
			while (offset < bufferBytes && !_wave.stopped())
			{
				const auto blocksToGenerate = std::min((bufferBytes - offset) / kBlockSize, _wave.maxAdvance());
				if (buffer)
				{
					const auto amplitude = _baseAmplitude * _wave._oscillator.stageSign();
					Generator generator{ _wave._oscillator.stageLength(), _wave._oscillator.stageOffset(), amplitude, _wave._modulator.currentOscillation() * amplitude };
					auto output = reinterpret_cast<float*>(static_cast<std::byte*>(buffer) + offset);
					auto amplitudeModulation = _wave._modulator.currentAmplitude();
					const auto step = _wave._modulator.currentAmplitudeStep();
					for (size_t i = 0; i < blocksToGenerate; ++i)
					{
						const auto value = static_cast<float>(generator() * amplitudeModulation);
						*output++ += value * _leftAmplitude;
						*output++ += value * _rightAmplitude;
						amplitudeModulation += step;
					}
				}
				_wave.advance(blocksToGenerate, _baseFrequency);
				offset += blocksToGenerate * kBlockSize;
			}
			return offset;
		}

		void restart() noexcept override
		{
			_wave.restart(_baseFrequency, 0);
		}

		unsigned samplingRate() const noexcept override
		{
			return _wave._oscillator.samplingRate();
		}

		void start(Note note, float amplitude) noexcept override
		{
			startImpl(note, amplitude);
			_wave.start(_baseFrequency, 0);
		}

		void stop() noexcept override
		{
			_wave.stop();
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
			, _leftWave{ _modulationData, samplingRate }
			, _rightWave{ _modulationData, samplingRate }
			, _phaseShift{ data._phaseShift }
		{
		}

		size_t render(void* buffer, size_t bufferBytes) noexcept override
		{
			bufferBytes -= bufferBytes % kBlockSize;
			size_t offset = 0;
			while (offset < bufferBytes && !(_leftWave.stopped() && _rightWave.stopped()))
			{
				const auto samplesToGenerate = std::min({ (bufferBytes - offset) / kBlockSize, _leftWave.maxAdvance(), _rightWave.maxAdvance() });
				if (buffer)
				{
					const auto leftAmplitude = _baseAmplitude * _leftWave._oscillator.stageSign() * _leftAmplitude;
					Generator leftGenerator{ _leftWave._oscillator.stageLength(), _leftWave._oscillator.stageOffset(), leftAmplitude, _leftWave._modulator.currentOscillation() * leftAmplitude };

					const auto rightAmplitude = _baseAmplitude * _rightWave._oscillator.stageSign() * _rightAmplitude;
					Generator rightGenerator{ _rightWave._oscillator.stageLength(), _rightWave._oscillator.stageOffset(), rightAmplitude, _rightWave._modulator.currentOscillation() * rightAmplitude };

					auto output = reinterpret_cast<float*>(static_cast<std::byte*>(buffer) + offset);
					auto leftModulation = _leftWave._modulator.currentAmplitude();
					const auto leftStep = _leftWave._modulator.currentAmplitudeStep();
					auto rightModulation = _rightWave._modulator.currentAmplitude();
					const auto rightStep = _rightWave._modulator.currentAmplitudeStep();
					for (size_t i = 0; i < samplesToGenerate; ++i)
					{
						*output++ += static_cast<float>(leftGenerator() * leftModulation);
						*output++ += static_cast<float>(rightGenerator() * rightModulation);
						leftModulation += leftStep;
						rightModulation += rightStep;
					}
				}
				_leftWave.advance(samplesToGenerate, _baseFrequency);
				_rightWave.advance(samplesToGenerate, _baseFrequency);
				offset += samplesToGenerate * kBlockSize;
			}
			return offset;
		}

		void restart() noexcept override
		{
			_leftWave.restart(_baseFrequency, 0);
			_rightWave.restart(_baseFrequency, _phaseShift);
		}

		unsigned samplingRate() const noexcept override
		{
			return _leftWave._oscillator.samplingRate();
		}

		void start(Note note, float amplitude) noexcept override
		{
			startImpl(note, amplitude);
			_leftWave.start(_baseFrequency, 0);
			_rightWave.start(_baseFrequency, _phaseShift);
		}

		void stop() noexcept override
		{
			_leftWave.stop();
			_rightWave.stop();
		}

	private:
		WaveState _leftWave;
		WaveState _rightWave;
		const float _phaseShift;
	};
}
