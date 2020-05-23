// This file is part of the Aulos toolkit.
// Copyright (C) Sergei Blagodarin.
// SPDX-License-Identifier: Apache-2.0

#include "composition.hpp"
#include "voice.hpp"

#include <algorithm>
#include <cstring>

static_assert(aulos::kMinSmoothCubicShape == aulos::SmoothCubicShaper::kMinShape);
static_assert(aulos::kMaxSmoothCubicShape == aulos::SmoothCubicShaper::kMaxShape);

static_assert(aulos::kMinQuinticShape == aulos::QuinticShaper::kMinShape);
static_assert(aulos::kMaxQuinticShape == aulos::QuinticShaper::kMaxShape);

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
			case aulos::WaveShape::SmoothQuadratic: return std::make_unique<aulos::MonoVoice<aulos::SmoothQuadraticShaper>>(data, samplingRate);
			case aulos::WaveShape::SharpQuadratic: return std::make_unique<aulos::MonoVoice<aulos::SharpQuadraticShaper>>(data, samplingRate);
			case aulos::WaveShape::SmoothCubic: return std::make_unique<aulos::MonoVoice<aulos::SmoothCubicShaper>>(data, samplingRate);
			case aulos::WaveShape::Quintic: return std::make_unique<aulos::MonoVoice<aulos::QuinticShaper>>(data, samplingRate);
			case aulos::WaveShape::Cosine: return std::make_unique<aulos::MonoVoice<aulos::CosineShaper>>(data, samplingRate);
			}
			break;

		case 2:
			switch (data._waveShape)
			{
			case aulos::WaveShape::Linear: return std::make_unique<aulos::StereoVoice<aulos::LinearShaper>>(data, samplingRate);
			case aulos::WaveShape::SmoothQuadratic: return std::make_unique<aulos::StereoVoice<aulos::SmoothQuadraticShaper>>(data, samplingRate);
			case aulos::WaveShape::SharpQuadratic: return std::make_unique<aulos::StereoVoice<aulos::SharpQuadraticShaper>>(data, samplingRate);
			case aulos::WaveShape::SmoothCubic: return std::make_unique<aulos::StereoVoice<aulos::SmoothCubicShaper>>(data, samplingRate);
			case aulos::WaveShape::Quintic: return std::make_unique<aulos::StereoVoice<aulos::QuinticShaper>>(data, samplingRate);
			case aulos::WaveShape::Cosine: return std::make_unique<aulos::StereoVoice<aulos::CosineShaper>>(data, samplingRate);
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
			const auto maxSequenceSpan = [](const aulos::Track& track) {
				unsigned result = 0;
				for (const auto& sequence : track._sequences)
				{
					if (sequence.empty())
						continue;
					unsigned current = 1;
					std::for_each(std::next(sequence.begin()), sequence.end(), [&result, &current](const aulos::Sound& sound) {
						if (sound._delay > 0)
						{
							if (current > result)
								result = current;
							current = 1;
						}
						else
							++current;
					});
					if (current > result)
						result = current;
				}
				return result;
			};

			size_t trackCount = 0;
			const auto totalWeight = static_cast<float>(std::accumulate(composition._parts.cbegin(), composition._parts.cend(), 0u, [&trackCount, &maxSequenceSpan](unsigned weight, const aulos::Part& part) {
				return weight + std::accumulate(part._tracks.cbegin(), part._tracks.cend(), 0u, [&trackCount, &maxSequenceSpan](unsigned partWeight, const aulos::Track& track) {
					const auto span = maxSequenceSpan(track);
					trackCount += span;
					return partWeight + track._weight * span;
				});
			}));
			_tracks.reserve(trackCount);
			for (const auto& part : composition._parts)
			{
				for (const auto& track : part._tracks)
				{
					const auto trackSpan = maxSequenceSpan(track);
					if (!trackSpan)
						continue;
					const auto trackWeight = track._weight / totalWeight;
					const auto trackState = &_tracks.emplace_back(part._voice, trackWeight, samplingRate, channels);
					for (unsigned i = 1; i < trackSpan; ++i)
						_tracks.emplace_back(part._voice, trackWeight, samplingRate, channels);
					assert(_tracks.size() <= trackCount);
					size_t fragmentOffset = 0;
					for (const auto& fragment : track._fragments)
					{
						fragmentOffset += fragment._delay;
						for (unsigned i = 0; i < trackSpan; ++i)
							trackState[i].removeStartingAt(fragmentOffset);
						if (const auto& sequence = track._sequences[fragment._sequence]; !sequence.empty())
						{
							auto soundOffset = fragmentOffset;
							auto trackIndex = ~0u;
							for (const auto& sound : sequence)
							{
								trackIndex = sound._delay > 0 ? 0 : trackIndex + 1;
								assert(trackIndex < trackSpan);
								soundOffset += sound._delay;
								trackState[trackIndex].addSound(soundOffset, sound._note);
							}
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
			bufferBytes -= bufferBytes % _blockBytes;
			if (buffer)
				std::memset(buffer, 0, bufferBytes);
			size_t offset = 0;
			for (auto& track : _tracks)
			{
				size_t trackOffset = 0;
				while (track._soundIndex != track._sounds.size())
				{
					if (!track._soundBytesRemaining)
					{
						const auto nextIndex = track._soundIndex + 1;
						track._soundBytesRemaining = nextIndex != track._sounds.size()
							? _stepBytes * track._sounds[nextIndex]._delay
							: track._voice->totalSamples() * _blockBytes;
						assert(track._soundBytesRemaining % _blockBytes == 0);
						track._voice->start(track._sounds[track._soundIndex]._note, track._normalizedWeight);
					}
					const auto bytesToGenerate = std::min(track._soundBytesRemaining, bufferBytes - trackOffset);
					if (!bytesToGenerate)
						break;
					[[maybe_unused]] const auto bytesGenerated = track._voice->render(buffer ? static_cast<std::byte*>(buffer) + trackOffset : nullptr, bytesToGenerate);
					assert(bytesGenerated <= bytesToGenerate); // Initial and inter-sound silence doesn't generate any data.
					track._soundBytesRemaining -= bytesToGenerate;
					trackOffset += bytesToGenerate;
					if (!track._soundBytesRemaining)
						++track._soundIndex;
				}
				offset = std::max(offset, trackOffset);
			}
			return offset;
		}

		void restart() noexcept override
		{
			for (auto& track : _tracks)
			{
				track._voice->stop();
				const auto delay = !track._sounds.empty() ? track._sounds.front()._delay : 0;
				track._soundIndex = delay ? std::numeric_limits<size_t>::max() : 0;
				track._soundBytesRemaining = _stepBytes * delay;
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
				if (!track._sounds.empty())
					result = std::max(result, track._lastSoundOffset * _stepSamples + track._voice->totalSamples());
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
			size_t _lastSoundOffset = 0;
			size_t _soundIndex = 0;
			size_t _soundBytesRemaining = 0;

			TrackState(const aulos::VoiceData& voiceData, float normalizedWeight, unsigned samplingRate, unsigned channels)
				: _voice{ ::createVoice(voiceData, samplingRate, channels) }
				, _normalizedWeight{ normalizedWeight }
			{
			}

			void addSound(size_t offset, aulos::Note note)
			{
				assert(offset > _lastSoundOffset || (offset == _lastSoundOffset && _sounds.empty()));
				_sounds.emplace_back(offset - _lastSoundOffset, note);
				_lastSoundOffset = offset;
			}

			void removeStartingAt(size_t offset) noexcept
			{
				if (offset > 0)
				{
					while (_lastSoundOffset >= offset)
					{
						assert(!_sounds.empty() && _lastSoundOffset >= _sounds.back()._delay);
						_lastSoundOffset -= _sounds.back()._delay;
						_sounds.pop_back();
					}
				}
				else
					_sounds.clear();
			}
		};

		const unsigned _samplingRate;
		const unsigned _channels;
		const size_t _blockBytes = _channels * sizeof(float);
		const size_t _stepSamples;
		const size_t _stepBytes = _stepSamples * _blockBytes;
		std::vector<TrackState> _tracks;
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
