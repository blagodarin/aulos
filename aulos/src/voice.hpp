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

#include "shaper.hpp"
#include "wave.hpp"

namespace aulos
{
	class VoiceImpl : public VoiceRenderer
	{
	public:
		VoiceImpl(const VoiceData& data, unsigned samplingRate) noexcept
			: _waveParameters{ data, samplingRate }
		{
		}

		virtual void stop() noexcept = 0;

	protected:
		const WaveParameters _waveParameters;
		float _baseAmplitude = 0.f;
	};

	template <typename Shaper>
	class MonoVoice final : public VoiceImpl
	{
	public:
		static constexpr auto kBlockSize = sizeof(float);

		MonoVoice(const VoiceData& data, unsigned samplingRate) noexcept
			: VoiceImpl{ data, samplingRate }
			, _wave{ _waveParameters, samplingRate, 0.f }
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
			while (offset < bufferBytes && !_wave.stopped())
			{
				const auto blocksToGenerate = static_cast<unsigned>(std::min<size_t>((bufferBytes - offset) / kBlockSize, _wave.maxAdvance()));
				if (buffer)
				{
					const auto output = reinterpret_cast<float*>(static_cast<std::byte*>(buffer) + offset);
					auto waveShaper = _wave.createWaveShaper<Shaper>(_baseAmplitude);
					auto amplitudeShaper = _wave.createAmplitudeShaper();
					for (unsigned i = 0; i < blocksToGenerate; ++i)
						output[i] += waveShaper.advance() * amplitudeShaper.advance();
				}
				_wave.advance(blocksToGenerate);
				offset += blocksToGenerate * kBlockSize;
			}
			return offset;
		}

		void restart() noexcept override
		{
			_wave.restart();
		}

		unsigned samplingRate() const noexcept override
		{
			return _wave.samplingRate();
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

	class BasicStereoVoice : public VoiceImpl
	{
	public:
		static constexpr auto kBlockSize = 2 * sizeof(float);

		BasicStereoVoice(const VoiceData& data, unsigned samplingRate) noexcept
			: VoiceImpl{ data, samplingRate }
			, _leftAmplitude{ std::min(1.f - data._stereoPan, 1.f) }
			, _rightAmplitude{ std::copysign(std::min(1.f + data._stereoPan, 1.f), data._stereoInversion ? -1.f : 1.f) }
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

	template <typename Shaper>
	class StereoVoice final : public BasicStereoVoice
	{
	public:
		StereoVoice(const VoiceData& data, unsigned samplingRate) noexcept
			: BasicStereoVoice{ data, samplingRate }
			, _wave{ _waveParameters, samplingRate, 0.f }
		{
		}

		size_t render(void* buffer, size_t bufferBytes) noexcept override
		{
			bufferBytes -= bufferBytes % kBlockSize;
			size_t offset = 0;
			while (offset < bufferBytes && !_wave.stopped())
			{
				const auto blocksToGenerate = static_cast<unsigned>(std::min<size_t>((bufferBytes - offset) / kBlockSize, _wave.maxAdvance()));
				if (buffer)
				{
					auto output = reinterpret_cast<float*>(static_cast<std::byte*>(buffer) + offset);
					auto waveShaper = _wave.createWaveShaper<Shaper>(_baseAmplitude);
					auto amplitudeShaper = _wave.createAmplitudeShaper();
					for (unsigned i = 0; i < blocksToGenerate; ++i)
					{
						const auto value = waveShaper.advance() * amplitudeShaper.advance();
						*output++ += value * _leftAmplitude;
						*output++ += value * _rightAmplitude;
					}
				}
				_wave.advance(blocksToGenerate);
				offset += blocksToGenerate * kBlockSize;
			}
			return offset;
		}

		void restart() noexcept override
		{
			_wave.restart();
		}

		unsigned samplingRate() const noexcept override
		{
			return _wave.samplingRate();
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
	class PhasedStereoVoice final : public BasicStereoVoice
	{
	public:
		PhasedStereoVoice(const VoiceData& data, unsigned samplingRate) noexcept
			: BasicStereoVoice{ data, samplingRate }
			, _leftWave{ _waveParameters, samplingRate, std::max(0.f, -data._stereoDelay) }
			, _rightWave{ _waveParameters, samplingRate, std::max(0.f, data._stereoDelay) }
		{
		}

		size_t render(void* buffer, size_t bufferBytes) noexcept override
		{
			bufferBytes -= bufferBytes % kBlockSize;
			size_t offset = 0;
			while (offset < bufferBytes && !(_leftWave.stopped() && _rightWave.stopped()))
			{
				const auto samplesToGenerate = static_cast<unsigned>(std::min<size_t>({ (bufferBytes - offset) / kBlockSize, _leftWave.maxAdvance(), _rightWave.maxAdvance() }));
				if (buffer)
				{
					auto output = reinterpret_cast<float*>(static_cast<std::byte*>(buffer) + offset);
					auto leftWaveShaper = _leftWave.createWaveShaper<Shaper>(_baseAmplitude * _leftAmplitude);
					auto rightWaveShaper = _rightWave.createWaveShaper<Shaper>(_baseAmplitude * _rightAmplitude);
					auto leftAmplitudeShaper = _leftWave.createAmplitudeShaper();
					auto rightAmplitudeShaper = _rightWave.createAmplitudeShaper();
					for (unsigned i = 0; i < samplesToGenerate; ++i)
					{
						*output++ += leftWaveShaper.advance() * leftAmplitudeShaper.advance();
						*output++ += rightWaveShaper.advance() * rightAmplitudeShaper.advance();
					}
				}
				_leftWave.advance(samplesToGenerate);
				_rightWave.advance(samplesToGenerate);
				offset += samplesToGenerate * kBlockSize;
			}
			return offset;
		}

		void restart() noexcept override
		{
			_leftWave.restart();
			_rightWave.restart();
		}

		unsigned samplingRate() const noexcept override
		{
			return _leftWave.samplingRate();
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
	};
}
