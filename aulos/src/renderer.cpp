// This file is part of the Aulos toolkit.
// Copyright (C) Sergei Blagodarin.
// SPDX-License-Identifier: Apache-2.0

#include "composition.hpp"
#include "voice.hpp"

#include <algorithm>
#include <cstring>
#include <functional>

static_assert(aulos::kMinSmoothCubicShape == aulos::SmoothCubicShaper::kMinShape);
static_assert(aulos::kMaxSmoothCubicShape == aulos::SmoothCubicShaper::kMaxShape);

static_assert(aulos::kMinQuinticShape == aulos::QuinticShaper::kMinShape);
static_assert(aulos::kMaxQuinticShape == aulos::QuinticShaper::kMaxShape);

namespace
{
	std::unique_ptr<aulos::Voice> createVoice(const aulos::VoiceData& data, unsigned samplingRate, unsigned channels)
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
		RendererImpl(const aulos::CompositionImpl& composition, unsigned samplingRate, unsigned channels, bool looping)
			: _samplingRate{ samplingRate }
			, _channels{ channels }
			, _stepSamples{ static_cast<size_t>(std::lround(static_cast<double>(samplingRate) / composition._speed)) }
			, _loopOffset{ composition._loopOffset }
		{
			struct TrackInfo
			{
				size_t _index = 0;
				size_t _samplesTotal = 0;
				size_t _samplesPlayed = 0;
				aulos::Note _note = aulos::Note::C0;

				constexpr TrackInfo(size_t index) noexcept
					: _index{ index } {}
			};

			std::vector<TrackInfo> currentTracks;
			std::vector<aulos::Sound> currentTrackSounds;
			for (const auto& part : composition._parts)
			{
				const auto matcher = [&part]() -> std::function<bool(const TrackInfo&, aulos::Note)> {
					switch (part._voice._polyphony)
					{
					case aulos::Polyphony::Full: return [](const TrackInfo& info, aulos::Note note) { return info._samplesPlayed == info._samplesTotal || info._note == note; };
					default: return [](const TrackInfo& info, aulos::Note note) { return info._samplesPlayed > 0 || info._note == note; };
					}
				}();
				currentTracks.clear();
				for (const auto& track : part._tracks)
				{
					currentTrackSounds.clear();
					{
						size_t lastSoundOffset = 0;
						size_t fragmentOffset = 0;
						for (const auto& fragment : track._fragments)
						{
							fragmentOffset += fragment._delay;
							if (fragmentOffset > 0)
							{
								while (lastSoundOffset >= fragmentOffset)
								{
									assert(!currentTrackSounds.empty());
									const auto lastSoundDelay = currentTrackSounds.back()._delay;
									assert(lastSoundOffset >= lastSoundDelay);
									lastSoundOffset -= lastSoundDelay;
									currentTrackSounds.pop_back();
								}
							}
							else
								currentTrackSounds.clear();
							auto soundOffset = fragmentOffset;
							for (const auto& sound : track._sequences[fragment._sequence])
							{
								soundOffset += sound._delay;
								assert(soundOffset >= lastSoundOffset);
								currentTrackSounds.emplace_back(soundOffset - lastSoundOffset, sound._note);
								lastSoundOffset = soundOffset;
							}
						}
					}
					size_t soundOffset = 0;
					for (const auto& sound : currentTrackSounds)
					{
						const auto delaySamples = sound._delay * _stepSamples;
						for (auto& currentTrack : currentTracks)
							currentTrack._samplesPlayed += std::min(currentTrack._samplesTotal - currentTrack._samplesPlayed, delaySamples);
						auto i = std::find_if(currentTracks.begin(), currentTracks.end(), [&matcher, &sound](const TrackInfo& info) { return matcher(info, sound._note); });
						if (i == currentTracks.end())
						{
							currentTracks.emplace_back(_tracks.size());
							_tracks.emplace_back(part._voice, track._weight, samplingRate, channels);
							i = std::prev(currentTracks.end());
						}
						auto& trackState = _tracks[i->_index];
						soundOffset += sound._delay;
						trackState.addSound(soundOffset, sound._note);
						i->_samplesTotal = trackState._voice->totalSamples();
						i->_samplesPlayed = 0;
						i->_note = sound._note;
					}
				}
			}
			_loopLength = composition._loopLength > 0 ? composition._loopLength : (totalSamples() + _stepSamples - 1) / _stepSamples;
			if (looping)
			{
				_looping = true;
				for (auto& track : _tracks)
					track.setLoop(_loopOffset, _loopLength);
			}
			restart(1);
		}

		std::pair<size_t, size_t> loopRange() const noexcept override
		{
			return { _loopOffset * _stepSamples, (_loopOffset + _loopLength) * _stepSamples };
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
						if (_looping)
						{
							if (track._soundIndex == track._loopEndIndex)
							{
								assert(track._loopBeginIndex == track._loopEndIndex);
								if (!track._loopDelay)
									break; // An infinite loop of silence.
								track._soundBytesRemaining = _stepBytes * track._loopDelay;
							}
							else
							{
								const auto nextIndex = track._soundIndex + 1;
								if (nextIndex == track._loopEndIndex)
								{
									track._soundBytesRemaining = track._loopDelay > 0
										? _stepBytes * track._loopDelay
										: track._voice->totalSamples() * _blockBytes;
								}
								else
								{
									assert(nextIndex != track._sounds.size());
									track._soundBytesRemaining = _stepBytes * track._sounds[nextIndex]._delay;
								}
							}
						}
						else
						{
							const auto nextIndex = track._soundIndex + 1;
							track._soundBytesRemaining = nextIndex != track._sounds.size()
								? _stepBytes * track._sounds[nextIndex]._delay
								: track._voice->totalSamples() * _blockBytes;
						}
						assert(track._soundBytesRemaining > 0);
						assert(track._soundBytesRemaining % _blockBytes == 0);
						track._voice->start(track._sounds[track._soundIndex]._note, track._gain);
					}
					const auto bytesToGenerate = std::min(track._soundBytesRemaining, bufferBytes - trackOffset);
					if (!bytesToGenerate)
						break;
					[[maybe_unused]] const auto bytesGenerated = track._voice->render(buffer ? reinterpret_cast<float*>(static_cast<std::byte*>(buffer) + trackOffset) : nullptr, bytesToGenerate);
					assert(bytesGenerated <= bytesToGenerate); // Initial and inter-sound silence doesn't generate any data.
					track._soundBytesRemaining -= bytesToGenerate;
					trackOffset += bytesToGenerate;
					if (!track._soundBytesRemaining && !(_looping && track._soundIndex == track._loopEndIndex && track._loopBeginIndex == track._loopEndIndex))
					{
						const auto nextIndex = track._soundIndex + 1;
						track._soundIndex = nextIndex == track._loopEndIndex ? track._loopBeginIndex : nextIndex;
					}
				}
				offset = std::max(offset, trackOffset);
			}
			return offset;
		}

		void restart(float gain) noexcept override
		{
			assert(gain >= 1);
			const auto totalWeight = std::accumulate(_tracks.begin(), _tracks.end(), 0.f, [](float weight, const TrackState& track) { return weight + track._weight; }) / gain;
			for (auto& track : _tracks)
			{
				track._voice->stop();
				track._gain = track._weight / totalWeight;
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
			std::unique_ptr<aulos::Voice> _voice;
			const float _weight;
			float _gain = 0.f;
			std::vector<TrackSound> _sounds;
			size_t _lastSoundOffset = 0;
			size_t _soundIndex = 0;
			size_t _soundBytesRemaining = 0;
			size_t _loopBeginIndex = 0;
			size_t _loopEndIndex = 0;
			size_t _loopDelay = 0;

			TrackState(const aulos::VoiceData& voiceData, unsigned weight, unsigned samplingRate, unsigned channels)
				: _voice{ ::createVoice(voiceData, samplingRate, channels) }
				, _weight{ static_cast<float>(weight) }
			{
			}

			void addSound(size_t offset, aulos::Note note)
			{
				assert(offset > _lastSoundOffset || (offset == _lastSoundOffset && _sounds.empty()));
				_sounds.emplace_back(offset - _lastSoundOffset, note);
				_lastSoundOffset = offset;
			}

			void setLoop(size_t loopOffset, size_t loopLength) noexcept
			{
				assert(!_sounds.empty());
				_loopBeginIndex = 0;
				size_t firstSoundOffset = _sounds[_loopBeginIndex]._delay;
				while (firstSoundOffset < loopOffset)
				{
					if (++_loopBeginIndex == _sounds.size())
					{
						_loopEndIndex = _loopBeginIndex;
						_loopDelay = 0;
						return; // Loop starts after the last sound of the track.
					}
					firstSoundOffset += _sounds[_loopBeginIndex]._delay;
				}
				_loopEndIndex = _loopBeginIndex;
				const auto firstSoundLoopOffset = firstSoundOffset - loopOffset;
				if (firstSoundLoopOffset >= loopLength)
				{
					_loopDelay = 0;
					return; // Loop ends before the first sound of the track.
				}
				auto lastSoundLoopOffset = firstSoundLoopOffset;
				for (;;)
				{
					++_loopEndIndex;
					if (_loopEndIndex == _sounds.size())
						break;
					const auto nextOffset = lastSoundLoopOffset + _sounds[_loopEndIndex]._delay;
					if (nextOffset >= loopLength)
						break;
					lastSoundLoopOffset = nextOffset;
				}
				_loopDelay = firstSoundLoopOffset + loopLength - lastSoundLoopOffset;
				assert(_loopDelay > 0);
			}
		};

		const unsigned _samplingRate;
		const unsigned _channels;
		const size_t _blockBytes = _channels * sizeof(float);
		const size_t _stepSamples;
		const size_t _stepBytes = _stepSamples * _blockBytes;
		const size_t _loopOffset;
		size_t _loopLength = 0;
		bool _looping = false;
		std::vector<TrackState> _tracks;
		float _gain = 1;
	};
}

namespace aulos
{
	std::unique_ptr<Renderer> Renderer::create(const Composition& composition, unsigned samplingRate, unsigned channels, bool looping)
	{
		return channels == 1 || channels == 2
			? std::make_unique<RendererImpl>(static_cast<const CompositionImpl&>(composition), samplingRate, channels, looping)
			: nullptr;
	}
}
