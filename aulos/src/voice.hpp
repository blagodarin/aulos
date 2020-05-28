// This file is part of the Aulos toolkit.
// Copyright (C) Sergei Blagodarin.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "wave.hpp"

namespace aulos
{
	class Voice
	{
	public:
		Voice(const VoiceData& data, unsigned samplingRate) noexcept
			: _waveData{ data, samplingRate }
		{
		}

		virtual ~Voice() noexcept = default;

		virtual size_t render(float* buffer, size_t bufferBytes) noexcept = 0;
		virtual void start(Note, float amplitude) noexcept = 0;
		virtual void stop() noexcept = 0;
		virtual size_t totalSamples() const noexcept = 0;

	protected:
		const WaveData _waveData;
		float _baseAmplitude = 0.f;
	};

	template <typename Shaper>
	class MonoVoice final : public Voice
	{
	public:
		static constexpr auto kBlockSize = sizeof(float);

		MonoVoice(const VoiceData& data, unsigned samplingRate) noexcept
			: Voice{ data, samplingRate }
			, _wave{ _waveData, samplingRate, 0.f }
		{
		}

		size_t render(float* buffer, size_t bufferBytes) noexcept override
		{
			assert(bufferBytes % kBlockSize == 0);
			size_t offset = 0;
			while (offset < bufferBytes && !_wave.stopped())
			{
				const auto blocksToGenerate = static_cast<unsigned>(std::min<size_t>((bufferBytes - offset) / kBlockSize, _wave.maxAdvance()));
				const auto output = buffer + offset / sizeof(float);
				Shaper waveShaper{ _wave.waveShaperData(_baseAmplitude) };
				LinearShaper amplitudeShaper{ _wave.amplitudeShaperData() };
				for (unsigned i = 0; i < blocksToGenerate; ++i)
					output[i] += waveShaper.advance() * amplitudeShaper.advance();
				_wave.advance(blocksToGenerate);
				offset += blocksToGenerate * kBlockSize;
			}
			return offset;
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

	template <typename Shaper>
	class StereoVoice final : public Voice
	{
	public:
		static constexpr auto kBlockSize = 2 * sizeof(float);

		StereoVoice(const VoiceData& data, unsigned samplingRate) noexcept
			: Voice{ data, samplingRate }
			, _leftWave{ _waveData, samplingRate, std::max(0.f, -data._stereoDelay) }
			, _rightWave{ _waveData, samplingRate, std::max(0.f, data._stereoDelay) }
			, _leftAmplitude{ std::min(1.f - data._stereoPan, 1.f) }
			, _rightAmplitude{ std::copysign(std::min(1.f + data._stereoPan, 1.f), data._stereoInversion ? -1.f : 1.f) }
		{
		}

		size_t render(float* buffer, size_t bufferBytes) noexcept override
		{
			assert(bufferBytes % kBlockSize == 0);
			size_t offset = 0;
			while (offset < bufferBytes && !(_leftWave.stopped() && _rightWave.stopped()))
			{
				const auto samplesToGenerate = static_cast<unsigned>(std::min<size_t>({ (bufferBytes - offset) / kBlockSize, _leftWave.maxAdvance(), _rightWave.maxAdvance() }));
				auto output = buffer + offset / sizeof(float);
				Shaper leftWaveShaper{ _leftWave.waveShaperData(_baseAmplitude * _leftAmplitude) };
				Shaper rightWaveShaper{ _rightWave.waveShaperData(_baseAmplitude * _rightAmplitude) };
				LinearShaper leftAmplitudeShaper{ _leftWave.amplitudeShaperData() };
				LinearShaper rightAmplitudeShaper{ _rightWave.amplitudeShaperData() };
				for (unsigned i = 0; i < samplesToGenerate; ++i)
				{
					*output++ += leftWaveShaper.advance() * leftAmplitudeShaper.advance();
					*output++ += rightWaveShaper.advance() * rightAmplitudeShaper.advance();
				}
				_leftWave.advance(samplesToGenerate);
				_rightWave.advance(samplesToGenerate);
				offset += samplesToGenerate * kBlockSize;
			}
			return offset;
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
		const float _leftAmplitude;
		const float _rightAmplitude;
	};
}
