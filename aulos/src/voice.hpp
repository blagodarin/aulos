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
			: _modulator{ data, samplingRate } {}

		void stop() noexcept { _modulator.stop(); }
		size_t totalSamples() const noexcept final { return _modulator.totalSamples(); }

	protected:
		void startImpl(Note note, float amplitude) noexcept;

	protected:
		float _baseAmplitude = 0.f;
		double _baseFrequency = 0.0;
		Modulator _modulator;
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
			, _oscillator{ samplingRate }
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
			while (offset < bufferBytes && !_modulator.stopped())
			{
				const auto blocksToGenerate = std::min({ (bufferBytes - offset) / kBlockSize, _modulator.maxAdvance(), _oscillator.maxAdvance() });
				if (buffer)
				{
					const auto amplitude = _baseAmplitude * _oscillator.stageSign();
					Generator generator{ _oscillator.stageLength(), _oscillator.stageOffset(), amplitude, _modulator.currentOscillation() * amplitude };
					const auto base = reinterpret_cast<float*>(static_cast<std::byte*>(buffer) + offset);
					auto amplitudeModulation = _modulator.currentAmplitude();
					const auto step = _modulator.currentAmplitudeStep();
					for (size_t i = 0; i < blocksToGenerate; ++i)
					{
						base[i] += static_cast<float>(generator() * amplitudeModulation);
						amplitudeModulation += step;
					}
				}
				_modulator.advance(blocksToGenerate);
				_oscillator.advance(blocksToGenerate, _baseFrequency * _modulator.currentFrequency(), _modulator.currentAsymmetry());
				offset += blocksToGenerate * kBlockSize;
			}
			return offset;
		}

		void restart() noexcept override
		{
			_modulator.stop();
			_modulator.start();
			_oscillator.restart(_baseFrequency * _modulator.currentFrequency(), _modulator.currentAsymmetry(), 0);
		}

		unsigned samplingRate() const noexcept override
		{
			return _oscillator.samplingRate();
		}

		void start(Note note, float amplitude) noexcept override
		{
			startImpl(note, amplitude);
			const bool wasStopped = _modulator.stopped();
			_modulator.start();
			const auto frequency = _baseFrequency * _modulator.currentFrequency();
			const auto asymmetry = _modulator.currentAsymmetry();
			if (wasStopped)
				_oscillator.restart(frequency, asymmetry, 0);
			else
				_oscillator.adjustStage(frequency, asymmetry);
		}

	private:
		Oscillator _oscillator;
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
			, _oscillator{ samplingRate }
		{
		}

		size_t render(void* buffer, size_t bufferBytes) noexcept override
		{
			bufferBytes -= bufferBytes % kBlockSize;
			size_t offset = 0;
			while (offset < bufferBytes && !_modulator.stopped())
			{
				const auto blocksToGenerate = std::min({ (bufferBytes - offset) / kBlockSize, _modulator.maxAdvance(), _oscillator.maxAdvance() });
				if (buffer)
				{
					const auto amplitude = _baseAmplitude * _oscillator.stageSign();
					Generator generator{ _oscillator.stageLength(), _oscillator.stageOffset(), amplitude, _modulator.currentOscillation() * amplitude };
					auto output = reinterpret_cast<float*>(static_cast<std::byte*>(buffer) + offset);
					auto amplitudeModulation = _modulator.currentAmplitude();
					const auto step = _modulator.currentAmplitudeStep();
					for (size_t i = 0; i < blocksToGenerate; ++i)
					{
						const auto value = static_cast<float>(generator() * amplitudeModulation);
						*output++ += value * _leftAmplitude;
						*output++ += value * _rightAmplitude;
						amplitudeModulation += step;
					}
				}
				_modulator.advance(blocksToGenerate);
				_oscillator.advance(blocksToGenerate, _baseFrequency * _modulator.currentFrequency(), _modulator.currentAsymmetry());
				offset += blocksToGenerate * kBlockSize;
			}
			return offset;
		}

		void restart() noexcept override
		{
			_modulator.stop();
			_modulator.start();
			_oscillator.restart(_baseFrequency * _modulator.currentFrequency(), _modulator.currentAsymmetry(), 0);
		}

		unsigned samplingRate() const noexcept override
		{
			return _oscillator.samplingRate();
		}

		void start(Note note, float amplitude) noexcept override
		{
			startImpl(note, amplitude);
			const bool wasStopped = _modulator.stopped();
			_modulator.start();
			const auto frequency = _baseFrequency * _modulator.currentFrequency();
			const auto asymmetry = _modulator.currentAsymmetry();
			if (wasStopped)
				_oscillator.restart(frequency, asymmetry, 0);
			else
				_oscillator.adjustStage(frequency, asymmetry);
		}

	private:
		Oscillator _oscillator;
	};

	template <typename Generator>
	class PhasedStereoVoice final : public BasicStereoVoice
	{
	public:
		PhasedStereoVoice(const VoiceData& data, unsigned samplingRate) noexcept
			: BasicStereoVoice{ data, samplingRate }
			, _leftOscillator{ samplingRate }
			, _rightOscillator{ samplingRate }
			, _phaseShift{ data._phaseShift }
		{
		}

		size_t render(void* buffer, size_t bufferBytes) noexcept override
		{
			bufferBytes -= bufferBytes % kBlockSize;
			size_t offset = 0;
			while (offset < bufferBytes && !_modulator.stopped())
			{
				const auto samplesToGenerate = std::min({ (bufferBytes - offset) / kBlockSize, _modulator.maxAdvance(), _leftOscillator.maxAdvance(), _rightOscillator.maxAdvance() });
				if (buffer)
				{
					const auto leftAmplitude = _baseAmplitude * _leftOscillator.stageSign() * _leftAmplitude;
					Generator leftGenerator{ _leftOscillator.stageLength(), _leftOscillator.stageOffset(), leftAmplitude, _modulator.currentOscillation() * leftAmplitude };

					const auto rightAmplitude = _baseAmplitude * _rightOscillator.stageSign() * _rightAmplitude;
					Generator rightGenerator{ _rightOscillator.stageLength(), _rightOscillator.stageOffset(), rightAmplitude, _modulator.currentOscillation() * rightAmplitude };

					auto output = reinterpret_cast<float*>(static_cast<std::byte*>(buffer) + offset);
					auto amplitudeModulation = _modulator.currentAmplitude();
					const auto step = _modulator.currentAmplitudeStep();
					for (size_t i = 0; i < samplesToGenerate; ++i)
					{
						*output++ += static_cast<float>(leftGenerator() * amplitudeModulation);
						*output++ += static_cast<float>(rightGenerator() * amplitudeModulation);
						amplitudeModulation += step;
					}
				}
				_modulator.advance(samplesToGenerate);
				const auto frequency = _baseFrequency * _modulator.currentFrequency();
				const auto asymmetry = _modulator.currentAsymmetry();
				_leftOscillator.advance(samplesToGenerate, frequency, asymmetry);
				_rightOscillator.advance(samplesToGenerate, frequency, asymmetry);
				offset += samplesToGenerate * kBlockSize;
			}
			return offset;
		}

		void restart() noexcept override
		{
			_modulator.stop();
			_modulator.start();
			const auto frequency = _baseFrequency * _modulator.currentFrequency();
			const auto asymmetry = _modulator.currentAsymmetry();
			_leftOscillator.restart(frequency, asymmetry, 0);
			_rightOscillator.restart(frequency, asymmetry, _phaseShift / _baseFrequency);
		}

		unsigned samplingRate() const noexcept override
		{
			return _leftOscillator.samplingRate();
		}

		void start(Note note, float amplitude) noexcept override
		{
			startImpl(note, amplitude);
			const bool wasStopped = _modulator.stopped();
			_modulator.start();
			const auto frequency = _baseFrequency * _modulator.currentFrequency();
			const auto asymmetry = _modulator.currentAsymmetry();
			if (wasStopped)
			{
				_leftOscillator.restart(frequency, asymmetry, 0);
				_rightOscillator.restart(frequency, asymmetry, _phaseShift / _baseFrequency);
			}
			else
			{
				_leftOscillator.adjustStage(frequency, asymmetry);
				_rightOscillator.adjustStage(frequency, asymmetry);
			}
		}

	private:
		Oscillator _leftOscillator;
		Oscillator _rightOscillator;
		const float _phaseShift;
	};
}
