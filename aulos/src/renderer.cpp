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
#include "oscillators.hpp"
#include "voice.hpp"

#include <algorithm>
#include <cassert>
#include <numeric>

namespace
{
	std::unique_ptr<aulos::VoiceImpl> createVoice(const aulos::VoiceData& data, unsigned samplingRate, unsigned channels)
	{
		switch (channels)
		{
		case 1:
			switch (data._wave)
			{
			case aulos::Wave::Linear: return std::make_unique<aulos::MonoVoice<aulos::LinearOscillator>>(data, samplingRate);
			case aulos::Wave::Quadratic: return std::make_unique<aulos::MonoVoice<aulos::QuadraticOscillator>>(data, samplingRate);
			case aulos::Wave::Cubic: return std::make_unique<aulos::MonoVoice<aulos::CubicOscillator>>(data, samplingRate);
			case aulos::Wave::Cosine: return std::make_unique<aulos::MonoVoice<aulos::CosineOscillator>>(data, samplingRate);
			}
			break;

		case 2:
			switch (data._wave)
			{
			case aulos::Wave::Linear: return std::make_unique<aulos::StereoVoice<aulos::LinearOscillator>>(data, samplingRate);
			case aulos::Wave::Quadratic: return std::make_unique<aulos::StereoVoice<aulos::QuadraticOscillator>>(data, samplingRate);
			case aulos::Wave::Cubic: return std::make_unique<aulos::StereoVoice<aulos::CubicOscillator>>(data, samplingRate);
			case aulos::Wave::Cosine: return std::make_unique<aulos::StereoVoice<aulos::CosineOscillator>>(data, samplingRate);
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
			, _stepBytes{ static_cast<size_t>(std::lround(static_cast<double>(samplingRate) / composition._speed)) * _blockBytes }
		{
			size_t trackCount = 0;
			const auto totalWeight = static_cast<float>(std::reduce(composition._parts.cbegin(), composition._parts.cend(), 0u, [&trackCount](unsigned weight, const aulos::Part& part) {
				return weight + std::reduce(part._tracks.cbegin(), part._tracks.cend(), 0u, [&trackCount](unsigned partWeight, const aulos::Track& track) {
					++trackCount;
					return partWeight + track._weight;
				});
			}));
			_tracks.reserve(trackCount);
			for (const auto& part : composition._parts)
			{
				for (const auto& track : part._tracks)
				{
					auto& trackSounds = _tracks.emplace_back(std::make_unique<TrackState>(part._voice, track._weight / totalWeight, samplingRate, channels))->_sounds;
					size_t fragmentOffset = 0;
					for (const auto& fragment : track._fragments)
					{
						fragmentOffset += fragment._delay;
						auto lastSoundOffset = std::reduce(trackSounds.cbegin(), trackSounds.cend(), size_t{}, [](size_t offset, const TrackSound& sound) { return offset + sound._delay; });
						while (!trackSounds.empty() && lastSoundOffset >= fragmentOffset)
						{
							lastSoundOffset -= trackSounds.back()._delay;
							trackSounds.pop_back();
						}
						if (const auto& sequence = track._sequences[fragment._sequence]; !sequence.empty())
						{
							trackSounds.reserve(trackSounds.size() + sequence.size());
							trackSounds.emplace_back(fragmentOffset - lastSoundOffset + sequence.front()._delay, sequence.front()._note);
							std::for_each(std::next(sequence.begin()), sequence.end(), [&trackSounds](const aulos::Sound& sound) {
								trackSounds.emplace_back(sound._delay, sound._note);
							});
						}
					}
				}
			}
			for (auto& track : _tracks)
				if (!track->_sounds.empty())
					if (const auto delay = track->_sounds.front()._delay; delay > 0)
					{
						track->_soundIndex = std::numeric_limits<size_t>::max();
						track->_soundBytesRemaining = _stepBytes * delay;
					}
		}

		unsigned channels() const noexcept
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
							: track->_voice->duration() * _blockBytes;
						assert(track->_soundBytesRemaining % _blockBytes == 0);
						track->_voice->start(track->_sounds[track->_soundIndex]._note, track->_normalizedWeight);
					}
					const auto bytesToGenerate = std::min(track->_soundBytesRemaining, bufferBytes - trackOffset);
					if (!bytesToGenerate)
						break;
					const auto bytesGenerated = track->_voice->render(buffer ? static_cast<std::byte*>(buffer) + trackOffset : nullptr, bytesToGenerate);
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

		unsigned samplingRate() const noexcept
		{
			return _samplingRate;
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
		};

		const unsigned _samplingRate;
		const unsigned _channels;
		const size_t _blockBytes = _channels * aulos::kSampleSize;
		const size_t _stepBytes;
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
