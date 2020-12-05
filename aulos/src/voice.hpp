// This file is part of the Aulos toolkit.
// Copyright (C) Sergei Blagodarin.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "wave.hpp"

#include <algorithm>

namespace aulos
{
	class Voice
	{
	public:
		Voice() noexcept = default;
		virtual ~Voice() noexcept = default;

		virtual void render(float* buffer, unsigned samples) noexcept = 0;
		virtual void start(Note, float amplitude, int delay) noexcept = 0;
		virtual void stop() noexcept = 0;

	protected:
		float _baseAmplitude = 0.f;
	};

	template <typename Shaper>
	class MonoVoice final : public Voice
	{
	public:
		MonoVoice(const WaveData& waveData, unsigned samplingRate) noexcept
			: _wave{ waveData, samplingRate }
		{
		}

		void render(float* buffer, unsigned samples) noexcept override
		{
			assert(samples > 0);
			do
			{
				assert(!_wave.stopped());
				Shaper waveShaper{ _wave.waveShaperData(_baseAmplitude) };
				LinearShaper amplitudeShaper{ _wave.amplitudeShaperData() };
				auto nextSamples = _wave.advance(samples);
				assert(nextSamples > 0);
				samples -= nextSamples;
				do
					*buffer++ += waveShaper.advance() * amplitudeShaper.advance();
				while (--nextSamples > 0);
			} while (samples > 0);
		}

		void start(Note note, float amplitude, int) noexcept override
		{
			_baseAmplitude = amplitude;
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
		StereoVoice(const WaveData& waveData, unsigned samplingRate) noexcept
			: _leftWave{ waveData, samplingRate }
			, _rightWave{ waveData, samplingRate }
		{
		}

		void render(float* buffer, unsigned samples) noexcept override
		{
			assert(samples > 0);
			do
			{
				assert(!(_leftWave.stopped() && _rightWave.stopped()));
				Shaper leftWaveShaper{ _leftWave.waveShaperData(_baseAmplitude) };
				LinearShaper leftAmplitudeShaper{ _leftWave.amplitudeShaperData() };
				auto leftSamples = _leftWave.advance(samples);
				assert(leftSamples > 0);
				samples -= leftSamples;
				do
				{
					Shaper rightWaveShaper{ _rightWave.waveShaperData(_baseAmplitude) };
					LinearShaper rightAmplitudeShaper{ _rightWave.amplitudeShaperData() };
					auto rightSamples = _rightWave.advance(leftSamples);
					assert(rightSamples > 0);
					leftSamples -= rightSamples;
					do
					{
						*buffer++ += leftWaveShaper.advance() * leftAmplitudeShaper.advance();
						*buffer++ += rightWaveShaper.advance() * rightAmplitudeShaper.advance();
					} while (--rightSamples > 0);
				} while (leftSamples > 0);
			} while (samples > 0);
		}

		void start(Note note, float amplitude, int delay) noexcept override
		{
			_baseAmplitude = amplitude;
			_leftWave.start(note, static_cast<unsigned>(std::max(delay, 0)));
			_rightWave.start(note, static_cast<unsigned>(std::max(-delay, 0)));
		}

		void stop() noexcept override
		{
			_leftWave.stop();
			_rightWave.stop();
		}

	private:
		WaveState _leftWave;
		WaveState _rightWave;
	};
}
