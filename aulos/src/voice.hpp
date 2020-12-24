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

		virtual unsigned render(float* buffer, unsigned maxSamples) noexcept = 0;
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

		unsigned render(float* buffer, unsigned maxSamples) noexcept override
		{
			assert(maxSamples > 0);
			auto remainingSamples = maxSamples;
			do
			{
				const auto maxAdvance = _wave.maxAdvance();
				assert(maxAdvance > 0);
				if (maxAdvance == std::numeric_limits<unsigned>::max())
					break;
				auto strideSamples = std::min(remainingSamples, maxAdvance);
				remainingSamples -= strideSamples;
				Shaper waveShaper{ _wave.waveShaperData(_baseAmplitude) };
				LinearShaper amplitudeShaper{ _wave.amplitudeShaperData() };
				_wave.advance(strideSamples);
				do
					*buffer++ += waveShaper.advance() * amplitudeShaper.advance();
				while (--strideSamples > 0);
			} while (remainingSamples > 0);
			return maxSamples - remainingSamples;
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

		unsigned render(float* buffer, unsigned maxSamples) noexcept override
		{
			assert(maxSamples > 0);
			auto remainingSamples = maxSamples;
			do
			{
				const auto maxAdvance = std::min(_leftWave.maxAdvance(), _rightWave.maxAdvance());
				assert(maxAdvance > 0);
				if (maxAdvance == std::numeric_limits<unsigned>::max())
					break;
				auto strideSamples = std::min(remainingSamples, maxAdvance);
				remainingSamples -= strideSamples;
				Shaper leftWaveShaper{ _leftWave.waveShaperData(_baseAmplitude) };
				LinearShaper leftAmplitudeShaper{ _leftWave.amplitudeShaperData() };
				_leftWave.advance(strideSamples);
				Shaper rightWaveShaper{ _rightWave.waveShaperData(_baseAmplitude) };
				LinearShaper rightAmplitudeShaper{ _rightWave.amplitudeShaperData() };
				_rightWave.advance(strideSamples);
				do
				{
					*buffer++ += leftWaveShaper.advance() * leftAmplitudeShaper.advance();
					*buffer++ += rightWaveShaper.advance() * rightAmplitudeShaper.advance();
				} while (--strideSamples > 0);
			} while (remainingSamples > 0);
			return maxSamples - remainingSamples;
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
