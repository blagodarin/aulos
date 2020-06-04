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
		Voice() noexcept = default;
		virtual ~Voice() noexcept = default;

		virtual void render(float* buffer, size_t samples) noexcept = 0;
		virtual void start(Note, float amplitude) noexcept = 0;
		virtual void stop() noexcept = 0;

	protected:
		float _baseAmplitude = 0.f;
	};

	template <typename Shaper>
	class MonoVoice final : public Voice
	{
	public:
		MonoVoice(const WaveData& waveData, const VoiceData&, unsigned samplingRate) noexcept
			: _wave{ waveData, samplingRate, 0 }
		{
		}

		void render(float* buffer, size_t samples) noexcept override
		{
			size_t offset = 0;
			while (offset < samples && !_wave.stopped())
			{
				const auto samplesToGenerate = static_cast<unsigned>(std::min<size_t>(samples - offset, _wave.maxAdvance()));
				const auto output = buffer + offset;
				Shaper waveShaper{ _wave.waveShaperData(_baseAmplitude) };
				LinearShaper amplitudeShaper{ _wave.amplitudeShaperData() };
				for (unsigned i = 0; i < samplesToGenerate; ++i)
					output[i] += waveShaper.advance() * amplitudeShaper.advance();
				_wave.advance(samplesToGenerate);
				offset += samplesToGenerate;
			}
			assert(offset == samples);
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

	private:
		WaveState _wave;
	};

	template <typename Shaper>
	class StereoVoice final : public Voice
	{
	public:
		StereoVoice(const WaveData& waveData, const VoiceData& voiceData, unsigned samplingRate) noexcept
			: _leftWave{ waveData, samplingRate, voiceData._stereoDelay < 0 ? waveData.absoluteDelay() : 0 }
			, _rightWave{ waveData, samplingRate, voiceData._stereoDelay > 0 ? waveData.absoluteDelay() : 0 }
			, _leftAmplitude{ std::min(1.f - voiceData._stereoPan, 1.f) }
			, _rightAmplitude{ std::copysign(std::min(1.f + voiceData._stereoPan, 1.f), voiceData._stereoInversion ? -1.f : 1.f) }
		{
		}

		void render(float* buffer, size_t samples) noexcept override
		{
			size_t offset = 0;
			while (offset < samples && !(_leftWave.stopped() && _rightWave.stopped()))
			{
				const auto samplesToGenerate = static_cast<unsigned>(std::min<size_t>({ samples - offset, _leftWave.maxAdvance(), _rightWave.maxAdvance() }));
				auto output = buffer + offset * 2;
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
				offset += samplesToGenerate;
			}
			assert(offset == samples);
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

	private:
		WaveState _leftWave;
		WaveState _rightWave;
		const float _leftAmplitude;
		const float _rightAmplitude;
	};
}
