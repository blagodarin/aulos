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

	struct AbsoluteSound
	{
		size_t _offset;
		aulos::Note _note;

		constexpr AbsoluteSound(size_t offset, aulos::Note note) noexcept
			: _offset{ offset }, _note{ note } {}
	};

	struct TrackSound
	{
		uint16_t _delaySteps;
		aulos::Note _note;
		uint8_t _chordLength;

		constexpr TrackSound(uint16_t delaySteps, aulos::Note note, uint8_t chordLength) noexcept
			: _delaySteps{ delaySteps }, _note{ note }, _chordLength{ chordLength } {}
	};

	struct TrackRenderer
	{
		struct ActiveSound
		{
			std::unique_ptr<aulos::Voice> _voice;
			size_t _bytesRemaining = 0;
			aulos::Note _note = aulos::Note::C0;

			ActiveSound(std::unique_ptr<aulos::Voice>&& voice, aulos::Note note) noexcept
				: _voice{ std::move(voice) }, _note{ note } {}
		};

		const float _weight;
		const aulos::Polyphony _polyphony;
		const size_t _soundBytes;
		std::vector<std::unique_ptr<aulos::Voice>> _voicePool;
		std::vector<ActiveSound> _activeSounds;
		std::vector<TrackSound> _sounds;
		std::vector<TrackSound>::const_iterator _nextSound = _sounds.cend();
		std::vector<TrackSound>::const_iterator _loopSound = _sounds.cend();
		size_t _lastSoundOffset = 0;
		size_t _loopDelay = 0;
		size_t _strideBytesRemaining = 0;
		float _gain = 0.f;

		explicit TrackRenderer(float weight, aulos::Polyphony polyphony, size_t soundBytes) noexcept
			: _weight{ weight }, _polyphony{ polyphony }, _soundBytes{ soundBytes } {}

		void setVoices(std::unique_ptr<aulos::Voice>&& firstVoice, size_t maxVoices, const aulos::VoiceData& voiceData, unsigned samplingRate, unsigned channels)
		{
			assert(_voicePool.empty() && _activeSounds.empty());
			_voicePool.reserve(maxVoices);
			_voicePool.emplace_back(std::move(firstVoice));
			while (_voicePool.size() < maxVoices)
				_voicePool.emplace_back(::createVoice(voiceData, samplingRate, channels));
			_activeSounds.reserve(maxVoices);
		}

		void setSounds(const std::vector<AbsoluteSound>& sounds)
		{
			assert(_sounds.empty());
			_sounds.reserve(sounds.size());
			_lastSoundOffset = 0;
			for (auto i = sounds.cbegin(); i != sounds.cend();)
			{
				auto delay = static_cast<uint16_t>(i->_offset - _lastSoundOffset);
				_lastSoundOffset = i->_offset;
				const auto chordEnd = std::find_if(std::next(i), sounds.cend(), [i](const AbsoluteSound& sound) { return sound._offset != i->_offset; });
				do
				{
					_sounds.emplace_back(delay, i->_note, static_cast<uint8_t>(chordEnd - i));
					delay = 0;
				} while (++i != chordEnd);
			}
			_nextSound = _sounds.cbegin();
			_loopSound = _sounds.cbegin();
			_loopDelay = 0;
		}

		void setLoop(size_t loopOffset, size_t loopLength)
		{
			assert(!_sounds.empty() && _loopSound == _sounds.cbegin() && !_loopDelay);
			auto loopSoundOffset = _loopSound->_delaySteps;
			while (loopSoundOffset < loopOffset)
			{
				if (++_loopSound == _sounds.cend())
					return;
				loopSoundOffset += _loopSound->_delaySteps;
			}
			const auto loopDistance = _lastSoundOffset - loopSoundOffset;
			assert(loopLength > loopDistance);
			_loopDelay = loopLength - loopDistance;
		}

		size_t render(void* buffer, size_t bufferBytes, size_t stepBytes) noexcept
		{
			size_t trackOffset = 0;
			while (trackOffset < bufferBytes)
			{
				if (!_strideBytesRemaining)
				{
					if (_nextSound == _sounds.cend())
					{
						if (_loopSound == _sounds.cend())
						{
							assert(!_loopDelay);
							trackOffset = bufferBytes;
						}
						break;
					}
					const auto chordLength = _nextSound->_chordLength;
					assert(chordLength > 0);
					assert(std::distance(_nextSound, _sounds.cend()) >= chordLength);
					const auto chordEnd = _nextSound + chordLength;
					assert(std::all_of(std::next(_nextSound), chordEnd, [](const TrackSound& sound) { return !sound._delaySteps; }));
					std::for_each(_nextSound, chordEnd, [this](const TrackSound& sound) {
						auto i = _activeSounds.end();
						switch (_polyphony)
						{
						case aulos::Polyphony::Chord:
							for (auto j = _activeSounds.begin(); j != _activeSounds.end(); ++j)
							{
								if (j->_bytesRemaining == _soundBytes)
									continue;
								if (i == _activeSounds.end() || i->_note < j->_note)
									i = j;
							}
							if (i == _activeSounds.end())
							{
								assert(!_voicePool.empty());
								i = _activeSounds.emplace(i, std::move(_voicePool.back()), sound._note);
								_voicePool.pop_back();
							}
							else
								i->_note = sound._note;
							break;
						case aulos::Polyphony::Full:
							i = std::find_if(_activeSounds.begin(), _activeSounds.end(), [&sound](const TrackRenderer::ActiveSound& activeSound) { return activeSound._note == sound._note; });
							if (i == _activeSounds.end())
							{
								assert(!_voicePool.empty());
								i = _activeSounds.emplace(i, std::move(_voicePool.back()), sound._note);
								_voicePool.pop_back();
							}
							break;
						}
						assert(i->_bytesRemaining < _soundBytes);
						i->_voice->start(i->_note, _gain);
						i->_bytesRemaining = _soundBytes;
					});
					_nextSound = chordEnd;
					if (_nextSound == _sounds.cend())
					{
						if (_loopDelay > 0)
						{
							_strideBytesRemaining = _loopDelay * stepBytes;
							_nextSound = _loopSound;
						}
						else
							_strideBytesRemaining = _soundBytes;
					}
					else
						_strideBytesRemaining = _nextSound->_delaySteps * stepBytes;
					assert(_strideBytesRemaining > 0);
				}
				const auto strideBytes = std::min(_strideBytesRemaining, bufferBytes - trackOffset);
				for (size_t i = 0; i < _activeSounds.size();)
				{
					auto& activeSound = _activeSounds[i];
					assert(activeSound._bytesRemaining > 0);
					const auto bytesToRender = std::min(activeSound._bytesRemaining, strideBytes);
					if (buffer)
						activeSound._voice->render(reinterpret_cast<float*>(static_cast<std::byte*>(buffer) + trackOffset), bytesToRender);
					activeSound._bytesRemaining -= bytesToRender;
					if (!activeSound._bytesRemaining)
					{
						_voicePool.emplace_back(std::move(activeSound._voice));
						if (auto j = _activeSounds.size() - 1; j != i)
							activeSound = std::move(_activeSounds[j]);
						_activeSounds.pop_back();
					}
					else
						++i;
				}
				_strideBytesRemaining -= strideBytes;
				trackOffset += strideBytes;
				if (_strideBytesRemaining > 0)
					break;
			}
			return trackOffset;
		}
	};

	class CompositionRenderer final : public aulos::Renderer
	{
	public:
		CompositionRenderer(const aulos::CompositionImpl& composition, unsigned samplingRate, unsigned channels, bool looping)
			: _samplingRate{ samplingRate }
			, _channels{ channels }
			, _stepSamples{ static_cast<size_t>(std::lround(static_cast<double>(samplingRate) / composition._speed)) }
			, _loopOffset{ composition._loopLength > 0 ? composition._loopOffset : 0 }
		{
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
					auto& trackRenderer = _trackRenderers.emplace_back(static_cast<float>(track._weight) / maxPolyphony, part._voice._polyphony, soundSamples * _blockBytes);
					trackRenderer.setVoices(std::move(voice), maxPolyphony, part._voice, samplingRate, channels);
					trackRenderer.setSounds(sounds);
				}
			}
			_loopLength = composition._loopLength > 0 ? composition._loopLength : (totalSamples() + _stepSamples - 1) / _stepSamples;
			if (looping)
				for (auto& trackRenderer : _trackRenderers)
					trackRenderer.setLoop(_loopOffset, _loopLength);
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
			for (auto& trackRenderer : _trackRenderers)
				offset = std::max(offset, trackRenderer.render(buffer, bufferBytes, _stepBytes));
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
				track._strideBytesRemaining = track._nextSound->_delaySteps * _stepBytes;
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
				result = std::max(result, track._lastSoundOffset * _stepSamples + track._soundBytes / _blockBytes);
			}
			return result;
		}

	public:
		const unsigned _samplingRate;
		const unsigned _channels;
		const size_t _blockBytes = _channels * sizeof(float);
		const size_t _stepSamples;
		const size_t _stepBytes = _stepSamples * _blockBytes;
		const size_t _loopOffset;
		size_t _loopLength = 0;
		std::vector<TrackRenderer> _trackRenderers;
	};
}

namespace aulos
{
	std::unique_ptr<Renderer> Renderer::create(const Composition& composition, unsigned samplingRate, unsigned channels, bool looping)
	{
		return channels == 1 || channels == 2
			? std::make_unique<CompositionRenderer>(static_cast<const CompositionImpl&>(composition), samplingRate, channels, looping)
			: nullptr;
	}
}
