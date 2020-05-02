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

#include "composition.hpp"
#include "voice.hpp"

#include <algorithm>
#include <cstring>

namespace
{
	std::unique_ptr<aulos::VoiceImpl> createVoice(const aulos::VoiceData& data, unsigned samplingRate, unsigned channels)
	{
		switch (channels)
		{
		case 1:
			switch (data._waveShape)
			{
			case aulos::WaveShape::Linear: return std::make_unique<aulos::MonoVoice<aulos::LinearShaper>>(data, samplingRate);
			case aulos::WaveShape::Quadratic1: return std::make_unique<aulos::MonoVoice<aulos::Quadratic1Shaper>>(data, samplingRate);
			case aulos::WaveShape::Quadratic2: return std::make_unique<aulos::MonoVoice<aulos::Quadratic2Shaper>>(data, samplingRate);
			case aulos::WaveShape::Cubic: return std::make_unique<aulos::MonoVoice<aulos::CubicShaper>>(data, samplingRate);
			case aulos::WaveShape::Quintic: return std::make_unique<aulos::MonoVoice<aulos::QuinticShaper>>(data, samplingRate);
			case aulos::WaveShape::Cosine: return std::make_unique<aulos::MonoVoice<aulos::CosineShaper>>(data, samplingRate);
			}
			break;

		case 2:
			if (data._stereoDelay == 0.f)
			{
				switch (data._waveShape)
				{
				case aulos::WaveShape::Linear: return std::make_unique<aulos::StereoVoice<aulos::LinearShaper>>(data, samplingRate);
				case aulos::WaveShape::Quadratic1: return std::make_unique<aulos::StereoVoice<aulos::Quadratic1Shaper>>(data, samplingRate);
				case aulos::WaveShape::Quadratic2: return std::make_unique<aulos::StereoVoice<aulos::Quadratic2Shaper>>(data, samplingRate);
				case aulos::WaveShape::Cubic: return std::make_unique<aulos::StereoVoice<aulos::CubicShaper>>(data, samplingRate);
				case aulos::WaveShape::Quintic: return std::make_unique<aulos::StereoVoice<aulos::QuinticShaper>>(data, samplingRate);
				case aulos::WaveShape::Cosine: return std::make_unique<aulos::StereoVoice<aulos::CosineShaper>>(data, samplingRate);
				}
			}
			else
			{
				switch (data._waveShape)
				{
				case aulos::WaveShape::Linear: return std::make_unique<aulos::PhasedStereoVoice<aulos::LinearShaper>>(data, samplingRate);
				case aulos::WaveShape::Quadratic1: return std::make_unique<aulos::PhasedStereoVoice<aulos::Quadratic1Shaper>>(data, samplingRate);
				case aulos::WaveShape::Quadratic2: return std::make_unique<aulos::PhasedStereoVoice<aulos::Quadratic2Shaper>>(data, samplingRate);
				case aulos::WaveShape::Cubic: return std::make_unique<aulos::PhasedStereoVoice<aulos::CubicShaper>>(data, samplingRate);
				case aulos::WaveShape::Quintic: return std::make_unique<aulos::PhasedStereoVoice<aulos::QuinticShaper>>(data, samplingRate);
				case aulos::WaveShape::Cosine: return std::make_unique<aulos::PhasedStereoVoice<aulos::CosineShaper>>(data, samplingRate);
				}
			}
			break;
		}
		return {};
	}

	class RendererImpl final : public aulos::Renderer
	{
	public:
		RendererImpl(const aulos::CompositionImpl& composition, unsigned samplingRate, unsigned channels)
			: _samplingRate{ samplingRate }
			, _channels{ channels }
			, _stepSamples{ static_cast<size_t>(std::lround(static_cast<double>(samplingRate) / composition._speed)) }
		{
			size_t trackCount = 0;
			const auto totalWeight = static_cast<float>(std::accumulate(composition._parts.cbegin(), composition._parts.cend(), 0u, [&trackCount](unsigned weight, const aulos::Part& part) {
				return weight + std::accumulate(part._tracks.cbegin(), part._tracks.cend(), 0u, [&trackCount](unsigned partWeight, const aulos::Track& track) {
					++trackCount;
					return partWeight + track._weight;
				});
			}));
			_tracks.reserve(trackCount);
			for (const auto& part : composition._parts)
			{
				for (const auto& track : part._tracks)
				{
					auto& trackState = *_tracks.emplace_back(std::make_unique<TrackState>(part._voice, track._weight / totalWeight, samplingRate, channels));
					size_t fragmentOffset = 0;
					for (const auto& fragment : track._fragments)
					{
						fragmentOffset += fragment._delay;
						auto lastSoundOffset = trackState.lastSoundOffset();
						while (!trackState._sounds.empty() && lastSoundOffset >= fragmentOffset)
						{
							lastSoundOffset -= trackState._sounds.back()._delay;
							trackState._sounds.pop_back();
						}
						if (const auto& sequence = track._sequences[fragment._sequence]; !sequence.empty())
						{
							trackState._sounds.reserve(trackState._sounds.size() + sequence.size());
							trackState._sounds.emplace_back(fragmentOffset - lastSoundOffset + sequence.front()._delay, sequence.front()._note);
							std::for_each(std::next(sequence.begin()), sequence.end(), [&trackState](const aulos::Sound& sound) {
								trackState._sounds.emplace_back(sound._delay, sound._note);
							});
						}
					}
				}
			}
			restart();
		}

		unsigned channels() const noexcept override
		{
			return _channels;
		}

		size_t render(void* buffer, size_t bufferBytes) noexcept override
		{
			if (buffer)
				std::memset(buffer, 0, bufferBytes);
			size_t offset = 0;
			for (auto& track : _tracks)
			{
				size_t trackOffset = 0;
				while (track->_soundIndex != track->_sounds.size())
				{
					if (!track->_soundBytesRemaining)
					{
						const auto nextIndex = track->_soundIndex + 1;
						track->_soundBytesRemaining = nextIndex != track->_sounds.size()
							? _stepBytes * track->_sounds[nextIndex]._delay
							: track->_voice->totalSamples() * _blockBytes;
						assert(track->_soundBytesRemaining % _blockBytes == 0);
						track->_voice->start(track->_sounds[track->_soundIndex]._note, track->_normalizedWeight);
					}
					const auto bytesToGenerate = std::min(track->_soundBytesRemaining, bufferBytes - trackOffset);
					if (!bytesToGenerate)
						break;
					[[maybe_unused]] const auto bytesGenerated = track->_voice->render(buffer ? static_cast<std::byte*>(buffer) + trackOffset : nullptr, bytesToGenerate);
					assert(bytesGenerated <= bytesToGenerate); // Initial and inter-sound silence doesn't generate any data.
					track->_soundBytesRemaining -= bytesToGenerate;
					trackOffset += bytesToGenerate;
					if (!track->_soundBytesRemaining)
						++track->_soundIndex;
				}
				offset = std::max(offset, trackOffset);
			}
			return offset;
		}

		void restart() noexcept override
		{
			for (auto& track : _tracks)
			{
				track->_voice->stop();
				const auto delay = !track->_sounds.empty() ? track->_sounds.front()._delay : 0;
				track->_soundIndex = delay ? std::numeric_limits<size_t>::max() : 0;
				track->_soundBytesRemaining = _stepBytes * delay;
			}
		}

		unsigned samplingRate() const noexcept override
		{
			return _samplingRate;
		}

		size_t totalSamples() const noexcept override
		{
			size_t result = 0;
			for (const auto& track : _tracks)
				if (!track->_sounds.empty())
					result = std::max(result, track->lastSoundOffset() * _stepSamples + track->_voice->totalSamples());
			return result;
		}

	public:
		struct TrackSound
		{
			size_t _delay = 0;
			aulos::Note _note = aulos::Note::C0;

			constexpr TrackSound(size_t delay, aulos::Note note) noexcept
				: _delay{ delay }, _note{ note } {}
		};

		struct TrackState
		{
			std::unique_ptr<aulos::VoiceImpl> _voice;
			const float _normalizedWeight;
			std::vector<TrackSound> _sounds;
			size_t _soundIndex = 0;
			size_t _soundBytesRemaining = 0;

			TrackState(const aulos::VoiceData& voiceData, float normalizedWeight, unsigned samplingRate, unsigned channels)
				: _voice{ ::createVoice(voiceData, samplingRate, channels) }
				, _normalizedWeight{ normalizedWeight }
			{
			}

			size_t lastSoundOffset() const noexcept
			{
				return std::accumulate(_sounds.cbegin(), _sounds.cend(), size_t{}, [](size_t offset, const TrackSound& sound) { return offset + sound._delay; });
			}
		};

		const unsigned _samplingRate;
		const unsigned _channels;
		const size_t _blockBytes = _channels * sizeof(float);
		const size_t _stepSamples;
		const size_t _stepBytes = _stepSamples * _blockBytes;
		std::vector<std::unique_ptr<TrackState>> _tracks;
	};
}

namespace aulos
{
	std::unique_ptr<VoiceRenderer> VoiceRenderer::create(const VoiceData& voiceData, unsigned samplingRate, unsigned channels)
	{
		return ::createVoice(voiceData, samplingRate, channels);
	}

	std::unique_ptr<Renderer> Renderer::create(const Composition& composition, unsigned samplingRate, unsigned channels)
	{
		return channels == 1 || channels == 2
			? std::make_unique<RendererImpl>(static_cast<const CompositionImpl&>(composition), samplingRate, channels)
			: nullptr;
	}
}
