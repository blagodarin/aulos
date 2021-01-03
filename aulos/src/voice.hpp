// This file is part of the Aulos toolkit.
// Copyright (C) Sergei Blagodarin.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "wave.hpp"

#include <algorithm>
#include <cassert>

namespace aulos
{
	class Voice
	{
	public:
		Voice() noexcept = default;
		virtual ~Voice() noexcept = default;

		virtual unsigned render(float* buffer, unsigned maxFrames) noexcept = 0;
		virtual void start(float frequency, float amplitude, int delay) noexcept = 0;
		virtual void stop() noexcept = 0;
	};

	template <typename Shaper>
	class MonoVoice final : public Voice
	{
	public:
		MonoVoice(const WaveData& waveData, unsigned samplingRate) noexcept
			: _wave{ waveData, samplingRate }
		{
		}

		unsigned render(float* buffer, unsigned maxFrames) noexcept override
		{
			assert(maxFrames > 0);
			auto remainingFrames = maxFrames;
			do
			{
				const auto maxAdvance = _wave.maxAdvance();
				assert(maxAdvance > 0);
				if (maxAdvance == std::numeric_limits<unsigned>::max())
					break;
				auto strideFrames = std::min(remainingFrames, maxAdvance);
				remainingFrames -= strideFrames;
				Shaper waveShaper{ _wave.waveShaperData() };
				LinearShaper amplitudeShaper{ _wave.amplitudeShaperData() };
				_wave.advance(strideFrames);
				do
					*buffer++ += waveShaper.advance() * amplitudeShaper.advance();
				while (--strideFrames > 0);
			} while (remainingFrames > 0);
			return maxFrames - remainingFrames;
		}

		void start(float frequency, float amplitude, int) noexcept override
		{
			_wave.start(frequency, amplitude, 0);
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

		unsigned render(float* buffer, unsigned maxFrames) noexcept override
		{
			assert(maxFrames > 0);
			auto remainingFrames = maxFrames;
			do
			{
				const auto maxAdvance = std::min(_leftWave.maxAdvance(), _rightWave.maxAdvance());
				assert(maxAdvance > 0);
				if (maxAdvance == std::numeric_limits<unsigned>::max())
					break;
				auto strideFrames = std::min(remainingFrames, maxAdvance);
				remainingFrames -= strideFrames;
				Shaper leftWaveShaper{ _leftWave.waveShaperData() };
				LinearShaper leftAmplitudeShaper{ _leftWave.amplitudeShaperData() };
				_leftWave.advance(strideFrames);
				Shaper rightWaveShaper{ _rightWave.waveShaperData() };
				LinearShaper rightAmplitudeShaper{ _rightWave.amplitudeShaperData() };
				_rightWave.advance(strideFrames);
				do
				{
					*buffer++ += leftWaveShaper.advance() * leftAmplitudeShaper.advance();
					*buffer++ += rightWaveShaper.advance() * rightAmplitudeShaper.advance();
				} while (--strideFrames > 0);
			} while (remainingFrames > 0);
			return maxFrames - remainingFrames;
		}

		void start(float frequency, float amplitude, int delay) noexcept override
		{
			_leftWave.start(frequency, amplitude, static_cast<unsigned>(std::max(delay, 0)));
			_rightWave.start(frequency, amplitude, static_cast<unsigned>(std::max(-delay, 0)));
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
