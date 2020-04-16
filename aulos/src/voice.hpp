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

namespace aulos
{
	constexpr auto kSampleSize = sizeof(float);

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

	class VoiceImpl : public VoiceRenderer
	{
	public:
		VoiceImpl(const VoiceData&, unsigned samplingRate) noexcept;

		unsigned samplingRate() const noexcept final { return _samplingRate; }
		void start(Note note, float amplitude) noexcept final;
		size_t totalSamples() const noexcept final { return _amplitudeModulator.duration(); }

	protected:
		void advance(size_t samples) noexcept;

	private:
		void updatePeriodParts() noexcept;

	protected:
		const unsigned _samplingRate;
		SampledEnvelope _amplitudeEnvelope;
		AmplitudeModulator _amplitudeModulator{ _amplitudeEnvelope };
		SampledEnvelope _frequencyEnvelope;
		LinearModulator _frequencyModulator{ _frequencyEnvelope };
		SampledEnvelope _asymmetryEnvelope;
		LinearModulator _asymmetryModulator{ _asymmetryEnvelope };
		SampledEnvelope _oscillationEnvelope;
		LinearModulator _oscillationModulator{ _oscillationEnvelope };
		float _amplitude = 0.f;
		double _frequency = 0.0;
		size_t _partIndex = 0;
		double _partLength = 0.0;
		double _partSamplesRemaining = 0.0;
	};

	template <unsigned kChannels>
	class BasicVoice : public VoiceImpl
	{
	public:
		static constexpr auto kBlockSize = kSampleSize * kChannels;

		using VoiceImpl::VoiceImpl;

		unsigned channels() const noexcept final { return kChannels; }
	};

	template <typename Oscillator>
	class MonoVoice final : public BasicVoice<1>
	{
	public:
		MonoVoice(const VoiceData& data, unsigned samplingRate) noexcept
			: BasicVoice{ data, samplingRate }
		{
		}

		size_t render(void* buffer, size_t bufferBytes) noexcept override
		{
			bufferBytes -= bufferBytes % kBlockSize;
			size_t offset = 0;
			while (offset < bufferBytes && _amplitudeModulator.update())
			{
				const auto samplesToGenerate = std::min(static_cast<size_t>(std::ceil(_partSamplesRemaining)), std::min((bufferBytes - offset) / kBlockSize, _amplitudeModulator.partSamplesRemaining()));
				if (buffer)
				{
					Oscillator oscillator{ _partLength, _partLength - _partSamplesRemaining, _amplitude, _oscillationModulator.value() * _amplitude };
					const auto base = reinterpret_cast<float*>(static_cast<std::byte*>(buffer) + offset);
					for (size_t i = 0; i < samplesToGenerate; ++i)
						base[i] += static_cast<float>(oscillator() * _amplitudeModulator.advance());
				}
				advance(samplesToGenerate);
				offset += samplesToGenerate * kBlockSize;
			}
			return offset;
		}
	};

	template <typename Oscillator>
	class StereoVoice final : public BasicVoice<2>
	{
	public:
		StereoVoice(const VoiceData& data, unsigned samplingRate) noexcept
			: BasicVoice{ data, samplingRate }
			, _leftAmplitude{ std::min(1.f - data._pan, 1.f) }
			, _rightAmplitude{ std::copysign(std::min(1.f + data._pan, 1.f), data._outOfPhase ? -1.f : 1.f) }
		{
		}

		size_t render(void* buffer, size_t bufferBytes) noexcept override
		{
			bufferBytes -= bufferBytes % kBlockSize;
			size_t offset = 0;
			while (offset < bufferBytes && _amplitudeModulator.update())
			{
				const auto samplesToGenerate = std::min(static_cast<size_t>(std::ceil(_partSamplesRemaining)), std::min((bufferBytes - offset) / kBlockSize, _amplitudeModulator.partSamplesRemaining()));
				if (buffer)
				{
					Oscillator oscillator{ _partLength, _partLength - _partSamplesRemaining, _amplitude, _oscillationModulator.value() * _amplitude };
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

	private:
		const float _leftAmplitude;
		const float _rightAmplitude;
	};
}
