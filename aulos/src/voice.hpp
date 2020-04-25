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
#include <cmath>

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

	private:
		size_t _size = 0;
		std::array<Point, 5> _points{};
	};

	class AmplitudeModulator
	{
	public:
		AmplitudeModulator(const SampledEnvelope&) noexcept;

		void start(double value) noexcept;
		double advance() noexcept;
		size_t duration() const noexcept { return _envelope.duration(); }
		size_t partSamplesRemaining() const noexcept;
		void stop() noexcept { _nextPoint = _envelope.end(); }
		bool stopped() const noexcept { return _nextPoint == _envelope.end(); }
		bool update() noexcept;
		constexpr double value() const noexcept { return _currentValue; }

	private:
		const SampledEnvelope& _envelope;
		double _baseValue = 0.f;
		const SampledEnvelope::Point* _nextPoint = _envelope.end();
		size_t _offset = 0;
		double _currentValue = 0.f;
	};

	class LinearModulator
	{
	public:
		LinearModulator(const SampledEnvelope&) noexcept;

		void start(double value) noexcept;
		void advance(size_t samples) noexcept;
		constexpr double value() const noexcept { return _currentValue; }

	private:
		const SampledEnvelope& _envelope;
		double _baseValue = 1.0;
		const SampledEnvelope::Point* _nextPoint = _envelope.end();
		size_t _offset = 0;
		double _currentValue = 0.0;
	};

	class Modulation
	{
	public:
		Modulation(const VoiceData&, unsigned samplingRate) noexcept;

		void advance(size_t samples) noexcept;
		constexpr auto currentAsymmetry() const noexcept { return _asymmetryModulator.value(); }
		constexpr auto currentFrequency() const noexcept { return _frequencyModulator.value(); }
		constexpr auto currentOscillation() const noexcept { return _oscillationModulator.value(); }
		auto maxAdvance() const noexcept { return _amplitudeModulator.partSamplesRemaining(); }
		auto nextAmplitude() noexcept { return _amplitudeModulator.advance(); }
		void start() noexcept;
		void stop() noexcept { _amplitudeModulator.stop(); }
		auto stopped() const noexcept { return _amplitudeModulator.stopped(); }
		auto totalSamples() const noexcept { return _amplitudeModulator.duration(); }
		bool update() noexcept { return _amplitudeModulator.update(); }

	private:
		const SampledEnvelope _amplitudeEnvelope;
		AmplitudeModulator _amplitudeModulator{ _amplitudeEnvelope };
		const SampledEnvelope _frequencyEnvelope;
		LinearModulator _frequencyModulator{ _frequencyEnvelope };
		const SampledEnvelope _asymmetryEnvelope;
		LinearModulator _asymmetryModulator{ _asymmetryEnvelope };
		const SampledEnvelope _oscillationEnvelope;
		LinearModulator _oscillationModulator{ _oscillationEnvelope };
	};

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
		void restart(double frequency, double asymmetry) noexcept;
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
		VoiceImpl(const VoiceData&, unsigned samplingRate) noexcept;

		void stop() noexcept { _modulation.stop(); }
		size_t totalSamples() const noexcept final { return _modulation.totalSamples(); }

	protected:
		void startImpl(Note note, float amplitude) noexcept;

	protected:
		float _baseAmplitude = 0.f;
		double _baseFrequency = 0.0;
		Modulation _modulation;
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
			while (offset < bufferBytes && _modulation.update())
			{
				const auto blocksToGenerate = std::min({ (bufferBytes - offset) / kBlockSize, _modulation.maxAdvance(), _oscillator.maxAdvance() });
				if (buffer)
				{
					const auto amplitude = _baseAmplitude * _oscillator.stageSign();
					Generator generator{ _oscillator.stageLength(), _oscillator.stageOffset(), amplitude, _modulation.currentOscillation() * amplitude };
					const auto base = reinterpret_cast<float*>(static_cast<std::byte*>(buffer) + offset);
					for (size_t i = 0; i < blocksToGenerate; ++i)
						base[i] += static_cast<float>(generator() * _modulation.nextAmplitude());
				}
				_modulation.advance(blocksToGenerate);
				_oscillator.advance(blocksToGenerate, _baseFrequency * _modulation.currentFrequency(), _modulation.currentAsymmetry());
				offset += blocksToGenerate * kBlockSize;
			}
			return offset;
		}

		void restart() noexcept override
		{
			_modulation.stop();
			_modulation.start();
			_oscillator.restart(_baseFrequency * _modulation.currentFrequency(), _modulation.currentAsymmetry());
		}

		unsigned samplingRate() const noexcept override
		{
			return _oscillator.samplingRate();
		}

		void start(Note note, float amplitude) noexcept override
		{
			startImpl(note, amplitude);
			const bool wasStopped = _modulation.stopped();
			_modulation.start();
			const auto frequency = _baseFrequency * _modulation.currentFrequency();
			const auto asymmetry = _modulation.currentAsymmetry();
			if (wasStopped)
				_oscillator.restart(frequency, asymmetry);
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
			while (offset < bufferBytes && _modulation.update())
			{
				const auto blocksToGenerate = std::min({ (bufferBytes - offset) / kBlockSize, _modulation.maxAdvance(), _oscillator.maxAdvance() });
				if (buffer)
				{
					const auto amplitude = _baseAmplitude * _oscillator.stageSign();
					Generator generator{ _oscillator.stageLength(), _oscillator.stageOffset(), amplitude, _modulation.currentOscillation() * amplitude };
					auto output = reinterpret_cast<float*>(static_cast<std::byte*>(buffer) + offset);
					for (size_t i = 0; i < blocksToGenerate; ++i)
					{
						const auto value = static_cast<float>(generator() * _modulation.nextAmplitude());
						*output++ += value * _leftAmplitude;
						*output++ += value * _rightAmplitude;
					}
				}
				_modulation.advance(blocksToGenerate);
				_oscillator.advance(blocksToGenerate, _baseFrequency * _modulation.currentFrequency(), _modulation.currentAsymmetry());
				offset += blocksToGenerate * kBlockSize;
			}
			return offset;
		}

		void restart() noexcept override
		{
			_modulation.stop();
			_modulation.start();
			_oscillator.restart(_baseFrequency * _modulation.currentFrequency(), _modulation.currentAsymmetry());
		}

		unsigned samplingRate() const noexcept override
		{
			return _oscillator.samplingRate();
		}

		void start(Note note, float amplitude) noexcept override
		{
			startImpl(note, amplitude);
			const bool wasStopped = _modulation.stopped();
			_modulation.start();
			const auto frequency = _baseFrequency * _modulation.currentFrequency();
			const auto asymmetry = _modulation.currentAsymmetry();
			if (wasStopped)
				_oscillator.restart(frequency, asymmetry);
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
		{
		}

		size_t render(void* buffer, size_t bufferBytes) noexcept override
		{
			bufferBytes -= bufferBytes % kBlockSize;
			size_t offset = 0;
			while (offset < bufferBytes && _modulation.update())
			{
				const auto samplesToGenerate = std::min({ (bufferBytes - offset) / kBlockSize, _modulation.maxAdvance(), _leftOscillator.maxAdvance(), _rightOscillator.maxAdvance() });
				if (buffer)
				{
					const auto leftAmplitude = _baseAmplitude * _leftOscillator.stageSign() * _leftAmplitude;
					Generator leftGenerator{ _leftOscillator.stageLength(), _leftOscillator.stageOffset(), leftAmplitude, _modulation.currentOscillation() * leftAmplitude };

					const auto rightAmplitude = _baseAmplitude * _rightOscillator.stageSign() * _rightAmplitude;
					Generator rightGenerator{ _rightOscillator.stageLength(), _rightOscillator.stageOffset(), rightAmplitude, _modulation.currentOscillation() * rightAmplitude };

					auto output = reinterpret_cast<float*>(static_cast<std::byte*>(buffer) + offset);
					for (size_t i = 0; i < samplesToGenerate; ++i)
					{
						const auto amplitudeModulation = _modulation.nextAmplitude();
						*output++ += static_cast<float>(leftGenerator() * amplitudeModulation);
						*output++ += static_cast<float>(rightGenerator() * amplitudeModulation);
					}
				}
				_modulation.advance(samplesToGenerate);
				const auto frequency = _baseFrequency * _modulation.currentFrequency();
				const auto asymmetry = _modulation.currentAsymmetry();
				_leftOscillator.advance(samplesToGenerate, frequency, asymmetry);
				_rightOscillator.advance(samplesToGenerate, frequency, asymmetry);
				offset += samplesToGenerate * kBlockSize;
			}
			return offset;
		}

		void restart() noexcept override
		{
			_modulation.stop();
			_modulation.start();
			const auto frequency = _baseFrequency * _modulation.currentFrequency();
			const auto asymmetry = _modulation.currentAsymmetry();
			_leftOscillator.restart(frequency, asymmetry);
			_rightOscillator.restart(frequency, asymmetry);
		}

		unsigned samplingRate() const noexcept override
		{
			return _leftOscillator.samplingRate();
		}

		void start(Note note, float amplitude) noexcept override
		{
			startImpl(note, amplitude);
			const bool wasStopped = _modulation.stopped();
			_modulation.start();
			const auto frequency = _baseFrequency * _modulation.currentFrequency();
			const auto asymmetry = _modulation.currentAsymmetry();
			if (wasStopped)
			{
				_leftOscillator.restart(frequency, asymmetry);
				_rightOscillator.restart(frequency, asymmetry);
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
	};
}
