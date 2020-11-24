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
		virtual void start(Note, float amplitude) noexcept = 0;
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

		void start(Note note, float amplitude) noexcept override
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

	struct CircularAcoustics
	{
		const float _headRadius = 0.f;   // In samples.
		const float _sourceRadius = 0.f; // In head radiuses.
		const float _sourceSize = 0.f;   // In right angles.
		const float _sourceOffset = 0.f; // In right angles, zero is forward, positive is right.

		constexpr CircularAcoustics() noexcept = default;

		constexpr CircularAcoustics(const TrackProperties& trackProperties, unsigned samplingRate) noexcept
			: _headRadius{ static_cast<float>(samplingRate) * trackProperties._headRadius / 1'000 }
			, _sourceRadius{ trackProperties._sourceRadius }
			, _sourceSize{ trackProperties._sourceSize }
			, _sourceOffset{ trackProperties._sourceOffset }
		{}

		int stereoDelay(Note note) const noexcept
		{
			constexpr int kLastNoteIndex = kNoteCount - 1;
			const auto noteAngle = static_cast<float>(2 * static_cast<int>(note) - kLastNoteIndex) / (2 * kLastNoteIndex);  // [-0.5, 0.5]
			const auto doubleSin = 2 * std::sin((noteAngle * _sourceSize + _sourceOffset) * std::numbers::pi_v<float> / 2); // [-2.0, 2.0]
			const auto leftDelay = std::sqrt(1 + _sourceRadius * (_sourceRadius + doubleSin));
			const auto rightDelay = std::sqrt(1 + _sourceRadius * (_sourceRadius - doubleSin));
			return static_cast<int>(_headRadius * (leftDelay - rightDelay));
		}
	};

	template <typename Shaper>
	class StereoVoice final : public Voice
	{
	public:
		StereoVoice(const WaveData& waveData, const CircularAcoustics& acoustics, const TrackProperties& trackProperties, unsigned samplingRate) noexcept
			: _leftWave{ waveData, samplingRate }
			, _rightWave{ waveData, samplingRate }
			, _leftAmplitude{ std::min(1.f - trackProperties._stereoPan, 1.f) }
			, _rightAmplitude{ std::copysign(std::min(1.f + trackProperties._stereoPan, 1.f), trackProperties._stereoInversion ? -1.f : 1.f) }
			, _acoustics{ acoustics }
		{
		}

		void render(float* buffer, unsigned samples) noexcept override
		{
			assert(samples > 0);
			do
			{
				assert(!(_leftWave.stopped() && _rightWave.stopped()));
				Shaper leftWaveShaper{ _leftWave.waveShaperData(_baseAmplitude * _leftAmplitude) };
				LinearShaper leftAmplitudeShaper{ _leftWave.amplitudeShaperData() };
				auto leftSamples = _leftWave.advance(samples);
				assert(leftSamples > 0);
				samples -= leftSamples;
				do
				{
					Shaper rightWaveShaper{ _rightWave.waveShaperData(_baseAmplitude * _rightAmplitude) };
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

		void start(Note note, float amplitude) noexcept override
		{
			const auto noteDelay = _acoustics.stereoDelay(note);
			_baseAmplitude = amplitude;
			_leftWave.start(note, static_cast<unsigned>(std::max(noteDelay, 0)));
			_rightWave.start(note, static_cast<unsigned>(std::max(-noteDelay, 0)));
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
		const CircularAcoustics& _acoustics;
	};
}
