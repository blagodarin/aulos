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

	// A wave period is represented by two stages.
	// The first stage (+1) starts at maximum amplitude and advances towards the minimum,
	// and the second stage (-1) starts at minimum amplitude and advances towards the maximum.
	// A stager holds the state of a current stage and controls switching between stages.
	class Stager
	{
	public:
		constexpr explicit Stager(unsigned samplingRate) noexcept
			: _samplingRate{ samplingRate } {}

		void startStage(double frequency, double asymmetry) noexcept;
		void adjustStage(double frequency, double asymmetry) noexcept;
		void advance(size_t samples, double nextFrequency, double nextAsymmetry) noexcept;
		auto samplesRemaining() const noexcept { return static_cast<size_t>(std::ceil(_stageRemainder)); }
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

		void restart() noexcept final;
		unsigned samplingRate() const noexcept final { return _stager.samplingRate(); }
		void start(Note note, float amplitude) noexcept final;
		void stop() noexcept { _amplitudeModulator.stop(); }
		size_t totalSamples() const noexcept final { return _amplitudeModulator.duration(); }

	protected:
		void advance(size_t samples) noexcept;

	private:
		void startImpl() noexcept;

	protected:
		const SampledEnvelope _amplitudeEnvelope;
		AmplitudeModulator _amplitudeModulator{ _amplitudeEnvelope };
		const SampledEnvelope _frequencyEnvelope;
		LinearModulator _frequencyModulator{ _frequencyEnvelope };
		const SampledEnvelope _asymmetryEnvelope;
		LinearModulator _asymmetryModulator{ _asymmetryEnvelope };
		const SampledEnvelope _oscillationEnvelope;
		LinearModulator _oscillationModulator{ _oscillationEnvelope };
		float _baseAmplitude = 0.f;
		double _baseFrequency = 0.0;
		Stager _stager;
	};

	// NOTE!
	// Keeping intermediate calculations in 'double' produces a measurable performance hit (e.g. 104ms -> 114 ms).

	template <typename Oscillator>
	class MonoVoice final : public VoiceImpl
	{
	public:
		static constexpr auto kBlockSize = sizeof(float);

		MonoVoice(const VoiceData& data, unsigned samplingRate) noexcept
			: VoiceImpl{ data, samplingRate }
		{
		}

		unsigned channels() const noexcept final
		{
			return 1;
		}

		size_t render(void* buffer, size_t bufferBytes) noexcept override
		{
			bufferBytes -= bufferBytes % kBlockSize;
			size_t offset = 0;
			while (offset < bufferBytes && _amplitudeModulator.update())
			{
				const auto blocksToGenerate = std::min(_stager.samplesRemaining(), std::min((bufferBytes - offset) / kBlockSize, _amplitudeModulator.partSamplesRemaining()));
				if (buffer)
				{
					const auto amplitude = _baseAmplitude * _stager.stageSign();
					Oscillator oscillator{ _stager.stageLength(), _stager.stageOffset(), amplitude, _oscillationModulator.value() * amplitude };
					const auto base = reinterpret_cast<float*>(static_cast<std::byte*>(buffer) + offset);
					for (size_t i = 0; i < blocksToGenerate; ++i)
						base[i] += static_cast<float>(oscillator() * _amplitudeModulator.advance());
				}
				advance(blocksToGenerate);
				offset += blocksToGenerate * kBlockSize;
			}
			return offset;
		}
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

	template <typename Oscillator>
	class StereoVoice final : public BasicStereoVoice
	{
	public:
		StereoVoice(const VoiceData& data, unsigned samplingRate) noexcept
			: BasicStereoVoice{ data, samplingRate }
		{
		}

		size_t render(void* buffer, size_t bufferBytes) noexcept override
		{
			bufferBytes -= bufferBytes % kBlockSize;
			size_t offset = 0;
			while (offset < bufferBytes && _amplitudeModulator.update())
			{
				const auto blocksToGenerate = std::min(_stager.samplesRemaining(), std::min((bufferBytes - offset) / kBlockSize, _amplitudeModulator.partSamplesRemaining()));
				if (buffer)
				{
					const auto amplitude = _baseAmplitude * _stager.stageSign();
					Oscillator oscillator{ _stager.stageLength(), _stager.stageOffset(), amplitude, _oscillationModulator.value() * amplitude };
					auto output = reinterpret_cast<float*>(static_cast<std::byte*>(buffer) + offset);
					for (size_t i = 0; i < blocksToGenerate; ++i)
					{
						const auto value = static_cast<float>(oscillator() * _amplitudeModulator.advance());
						*output++ += value * _leftAmplitude;
						*output++ += value * _rightAmplitude;
					}
				}
				advance(blocksToGenerate);
				offset += blocksToGenerate * kBlockSize;
			}
			return offset;
		}
	};

	template <typename Oscillator>
	class PhasedStereoVoice final : public BasicStereoVoice
	{
	public:
		PhasedStereoVoice(const VoiceData& data, unsigned samplingRate) noexcept
			: BasicStereoVoice{ data, samplingRate }
		{
		}

		size_t render(void* buffer, size_t bufferBytes) noexcept override
		{
			bufferBytes -= bufferBytes % kBlockSize;
			size_t offset = 0;
			while (offset < bufferBytes && _amplitudeModulator.update())
			{
				const auto samplesToGenerate = std::min(_stager.samplesRemaining(), std::min((bufferBytes - offset) / kBlockSize, _amplitudeModulator.partSamplesRemaining()));
				if (buffer)
				{
					const auto amplitude = _baseAmplitude * _stager.stageSign();
					Oscillator oscillator{ _stager.stageLength(), _stager.stageOffset(), amplitude, _oscillationModulator.value() * amplitude };
					auto output = reinterpret_cast<float*>(static_cast<std::byte*>(buffer) + offset);
					for (size_t i = 0; i < samplesToGenerate; ++i)
					{
						const auto value = static_cast<float>(oscillator() * _amplitudeModulator.advance());
						*output++ += value * _leftAmplitude;
						*output++ += value * _rightAmplitude;
					}
				}
				advance(samplesToGenerate);
				offset += samplesToGenerate * kBlockSize;
			}
			return offset;
		}
	};
}
