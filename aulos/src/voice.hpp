// This file is part of the Aulos toolkit.
// Copyright (C) Sergei Blagodarin.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "wave.hpp"

#include <algorithm>
#include <limits>
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
				const auto maxStrideFrames = _wave.prepareAdvance();
				assert(maxStrideFrames > 0);
				if (maxStrideFrames == std::numeric_limits<int>::max())
					break;
				auto strideFrames = std::min(remainingFrames, static_cast<unsigned>(maxStrideFrames));
				remainingFrames -= strideFrames;
				Shaper shaper{ _wave.waveShaperData() };
				_wave.advance(static_cast<int>(strideFrames));
				do
					*buffer++ += shaper.advance();
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
				const auto maxStrideFrames = std::min(_leftWave.prepareAdvance(), _rightWave.prepareAdvance());
				assert(maxStrideFrames > 0);
				if (maxStrideFrames == std::numeric_limits<int>::max())
					break;
				auto strideFrames = std::min(remainingFrames, static_cast<unsigned>(maxStrideFrames));
				remainingFrames -= strideFrames;
				Shaper leftShaper{ _leftWave.waveShaperData() };
				Shaper rightShaper{ _rightWave.waveShaperData() };
				_leftWave.advance(static_cast<int>(strideFrames));
				_rightWave.advance(static_cast<int>(strideFrames));
				do
				{
					*buffer++ += leftShaper.advance();
					*buffer++ += rightShaper.advance();
				} while (--strideFrames > 0);
			} while (remainingFrames > 0);
			return maxFrames - remainingFrames;
		}

		void start(float frequency, float amplitude, int delay) noexcept override
		{
			_leftWave.start(frequency, amplitude, std::max(delay, 0));
			_rightWave.start(frequency, amplitude, std::max(-delay, 0));
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
