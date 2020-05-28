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
			, _looping{ looping }
			, _loopOffset{ composition._loopLength > 0 ? composition._loopOffset : 0 }
		{
			struct AbsoluteSound
			{
				size_t _offset;
				aulos::Note _note;

				constexpr AbsoluteSound(size_t offset, aulos::Note note) noexcept
					: _offset{ offset }, _note{ note } {}
			};

			std::vector<AbsoluteSound> sounds;
			std::vector<aulos::Note> noteCounter;
			const auto loopEnd = looping && composition._loopLength > 0 ? size_t{ composition._loopOffset } + composition._loopLength : std::numeric_limits<size_t>::max();
			for (const auto& part : composition._parts)
			{
				for (const auto& track : part._tracks)
				{
					sounds.clear();
					for (size_t fragmentOffset = 0; const auto& fragment : track._fragments)
					{
						fragmentOffset += fragment._delay;
						if (fragmentOffset >= loopEnd)
							break;
						sounds.erase(std::find_if(sounds.crbegin(), sounds.crend(), [fragmentOffset](const AbsoluteSound& sound) { return sound._offset < fragmentOffset; }).base(), sounds.cend());
						for (auto soundOffset = fragmentOffset; const auto& sound : track._sequences[fragment._sequence])
						{
							soundOffset += sound._delay;
							if (soundOffset >= loopEnd)
								break;
							sounds.emplace_back(soundOffset, sound._note);
						}
					}
					if (sounds.empty())
						break;
					auto voice = ::createVoice(part._voice, samplingRate, channels);
					const auto soundSamples = voice->totalSamples();
					const auto soundSteps = (soundSamples + _stepSamples - 1) / _stepSamples;
					if (!soundSteps)
						break;
					size_t maxPolyphony = 0;
					switch (part._voice._polyphony)
					{
					case aulos::Polyphony::Chord:
						for (auto i = sounds.cbegin(); i != sounds.cend();)
						{
							const auto j = std::find_if(std::next(i), sounds.cend(), [offset = i->_offset](const AbsoluteSound& sound) { return sound._offset != offset; });
							maxPolyphony = std::max(maxPolyphony, static_cast<size_t>(j - i));
							i = j;
						}
						break;
					case aulos::Polyphony::Full:
						for (auto i = sounds.cbegin(); i != sounds.cend(); ++i)
						{
							noteCounter.clear();
							noteCounter.emplace_back(i->_note);
							std::for_each(i, std::find_if(std::next(i), sounds.cend(), [endOffset = i->_offset + soundSteps](const AbsoluteSound& sound) { return sound._offset >= endOffset; }),
								[&noteCounter](const AbsoluteSound& sound) {
									if (std::none_of(noteCounter.cbegin(), noteCounter.cend(), [&sound](aulos::Note note) { return sound._note == note; }))
										noteCounter.emplace_back(sound._note);
								});
							// TODO: Take looping into account.
							maxPolyphony = std::max(maxPolyphony, noteCounter.size());
						}
						break;
					}
					auto& trackRenderer = _trackRenderers.emplace_back(static_cast<float>(track._weight) / maxPolyphony, part._voice._polyphony, soundSamples);
					trackRenderer._voicePool.emplace_back(std::move(voice));
					while (trackRenderer._voicePool.size() < maxPolyphony)
						trackRenderer._voicePool.emplace_back(::createVoice(part._voice, samplingRate, channels));
					trackRenderer._activeSounds.reserve(maxPolyphony);
					trackRenderer._sounds.reserve(sounds.size());
					for (size_t currentOffset = 0; const auto& sound : sounds)
					{
						trackRenderer._sounds.emplace_back(sound._offset - currentOffset, sound._note);
						currentOffset = sound._offset;
					}
					trackRenderer._nextSound = trackRenderer._sounds.cbegin();
					const auto loopSound = std::find_if(sounds.cbegin(), sounds.cend(), [this](const AbsoluteSound& sound) { return sound._offset >= _loopOffset; });
					trackRenderer._loopSound = std::next(trackRenderer._sounds.cbegin(), loopSound - sounds.cbegin());
				}
			}
			_loopLength = composition._loopLength > 0 ? composition._loopLength : (totalSamples() + _stepSamples - 1) / _stepSamples;
			if (_looping)
			{
				for (auto& track : _trackRenderers)
				{
					if (track._loopSound == track._sounds.cend())
						continue;
					const auto boundary = std::next(track._loopSound);
					const auto trackLoopOffset = std::accumulate(track._sounds.cbegin(), boundary, size_t{}, [](size_t offset, const aulos::Sound& sound) { return offset + sound._delay; });
					const auto trackEndOffset = std::accumulate(boundary, track._sounds.cend(), trackLoopOffset, [](size_t offset, const aulos::Sound& sound) { return offset + sound._delay; });
					track._loopDelay = _loopLength - (trackEndOffset - trackLoopOffset);
				}
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
			for (auto& track : _trackRenderers)
			{
				size_t trackOffset = 0;
				while (trackOffset < bufferBytes)
				{
					if (!track._strideBytesRemaining)
					{
						if (track._nextSound == track._sounds.cend())
						{
							if (_looping)
							{
								assert(!track._loopDelay);
								trackOffset = bufferBytes;
							}
							break;
						}
						do
						{
							auto i = track._activeSounds.end();
							switch (track._polyphony)
							{
							case aulos::Polyphony::Chord:
								for (auto j = track._activeSounds.begin(); j != track._activeSounds.end(); ++j)
								{
									if (j->_bytesRemaining == track._soundSamples * _blockBytes)
										continue;
									if (i == track._activeSounds.end() || i->_note < j->_note)
										i = j;
								}
								if (i == track._activeSounds.end())
								{
									assert(!track._voicePool.empty());
									i = track._activeSounds.emplace(i, std::move(track._voicePool.back()), track._soundSamples * _blockBytes, track._nextSound->_note);
									track._voicePool.pop_back();
								}
								else
									i->_note = track._nextSound->_note;
								break;
							case aulos::Polyphony::Full:
								i = std::find_if(track._activeSounds.begin(), track._activeSounds.end(), [&track](const TrackRenderer::ActiveSound& sound) { return sound._note == track._nextSound->_note; });
								if (i == track._activeSounds.end())
								{
									assert(!track._voicePool.empty());
									i = track._activeSounds.emplace(i, std::move(track._voicePool.back()), track._soundSamples * _blockBytes, track._nextSound->_note);
									track._voicePool.pop_back();
								}
								break;
							}
							i->_voice->start(i->_note, track._gain);
							i->_bytesRemaining = track._soundSamples * _blockBytes;
							++track._nextSound;
						} while (track._nextSound != track._sounds.cend() && !track._nextSound->_delay);
						if (track._nextSound == track._sounds.cend())
						{
							if (track._loopDelay > 0)
							{
								track._strideBytesRemaining = track._loopDelay * _stepBytes;
								track._nextSound = track._loopSound;
							}
							else
								track._strideBytesRemaining = track._soundSamples * _blockBytes;
						}
						else
							track._strideBytesRemaining = track._nextSound->_delay * _stepBytes;
						assert(track._strideBytesRemaining > 0);
					}
					const auto strideBytes = std::min(track._strideBytesRemaining, bufferBytes - trackOffset);
					for (size_t i = 0; i < track._activeSounds.size();)
					{
						auto& activeSound = track._activeSounds[i];
						assert(activeSound._bytesRemaining > 0);
						const auto bytesToRender = std::min(activeSound._bytesRemaining, strideBytes);
						if (buffer)
						{
							[[maybe_unused]] const auto bytesRendered = activeSound._voice->render(reinterpret_cast<float*>(static_cast<std::byte*>(buffer) + trackOffset), bytesToRender);
							assert(bytesRendered == bytesToRender);
						}
						activeSound._bytesRemaining -= bytesToRender;
						if (!activeSound._bytesRemaining)
						{
							track._voicePool.emplace_back(std::move(activeSound._voice));
							if (auto j = track._activeSounds.size() - 1; j != i)
								activeSound = std::move(track._activeSounds[j]);
							track._activeSounds.pop_back();
						}
						else
							++i;
					}
					track._strideBytesRemaining -= strideBytes;
					trackOffset += strideBytes;
					if (track._strideBytesRemaining > 0)
						break;
				}
				offset = std::max(offset, trackOffset);
			}
			return offset;
		}

		void restart(float gain) noexcept override
		{
			assert(gain >= 1);
			for (auto& track : _trackRenderers)
			{
				for (auto& activeVoice : track._activeSounds)
				{
					activeVoice._voice->stop();
					track._voicePool.emplace_back(std::move(activeVoice._voice));
				}
				track._activeSounds.clear();
				track._nextSound = track._sounds.cbegin();
				track._strideBytesRemaining = track._nextSound->_delay * _stepBytes;
				track._gain = track._weight * gain;
			}
		}

		unsigned samplingRate() const noexcept override
		{
			return _samplingRate;
		}

		size_t totalSamples() const noexcept override
		{
			size_t result = 0;
			for (const auto& track : _trackRenderers)
			{
				assert(!track._sounds.empty());
				result = std::max(result, std::accumulate(track._sounds.cbegin(), track._sounds.cend(), size_t{}, [](size_t offset, const aulos::Sound& sound) { return offset + sound._delay; }) * _stepSamples + track._soundSamples);
			}
			return result;
		}

	public:
		struct TrackRenderer
		{
			struct ActiveSound
			{
				std::unique_ptr<aulos::Voice> _voice;
				size_t _bytesRemaining = 0;
				aulos::Note _note = aulos::Note::C0;

				ActiveSound(std::unique_ptr<aulos::Voice>&& voice, size_t bytesRemaining, aulos::Note note) noexcept
					: _voice{ std::move(voice) }, _bytesRemaining{ bytesRemaining }, _note{ note } {}
			};

			const float _weight;
			const aulos::Polyphony _polyphony;
			const size_t _soundSamples;
			std::vector<std::unique_ptr<aulos::Voice>> _voicePool;
			std::vector<ActiveSound> _activeSounds;
			std::vector<aulos::Sound> _sounds;
			std::vector<aulos::Sound>::const_iterator _nextSound = _sounds.cend();
			std::vector<aulos::Sound>::const_iterator _loopSound = _sounds.cend();
			size_t _loopDelay = 0;
			size_t _strideBytesRemaining = 0;
			float _gain = 0.f;

			explicit TrackRenderer(float weight, aulos::Polyphony polyphony, size_t soundSamples) noexcept
				: _weight{ weight }, _polyphony{ polyphony }, _soundSamples{ soundSamples } {}
		};

		const unsigned _samplingRate;
		const unsigned _channels;
		const size_t _blockBytes = _channels * sizeof(float);
		const size_t _stepSamples;
		const size_t _stepBytes = _stepSamples * _blockBytes;
		const bool _looping;
		const size_t _loopOffset;
		size_t _loopLength = 0;
		std::vector<TrackRenderer> _trackRenderers;
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
