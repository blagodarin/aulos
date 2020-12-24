// This file is part of the Aulos toolkit.
// Copyright (C) Sergei Blagodarin.
// SPDX-License-Identifier: Apache-2.0

#include "acoustics.hpp"
#include "composition.hpp"
#include "utils/limited_vector.hpp"
#include "utils/static_vector.hpp"
#include "voice.hpp"

#include <algorithm>
#include <cstring>
#include <functional>
#include <optional>

namespace
{
	std::unique_ptr<aulos::Voice> createVoice(const aulos::WaveData& waveData, const aulos::VoiceData& voiceData, unsigned samplingRate, bool isStereo)
	{
		if (!isStereo)
		{
			switch (voiceData._waveShape)
			{
			case aulos::WaveShape::Linear: return std::make_unique<aulos::MonoVoice<aulos::LinearShaper>>(waveData, samplingRate);
			case aulos::WaveShape::Quadratic: return std::make_unique<aulos::MonoVoice<aulos::QuadraticShaper>>(waveData, samplingRate);
			case aulos::WaveShape::Cubic: return std::make_unique<aulos::MonoVoice<aulos::CubicShaper>>(waveData, samplingRate);
			case aulos::WaveShape::Quintic: return std::make_unique<aulos::MonoVoice<aulos::QuinticShaper>>(waveData, samplingRate);
			case aulos::WaveShape::Cosine: return std::make_unique<aulos::MonoVoice<aulos::CosineShaper>>(waveData, samplingRate);
			}
		}
		else
		{
			switch (voiceData._waveShape)
			{
			case aulos::WaveShape::Linear: return std::make_unique<aulos::StereoVoice<aulos::LinearShaper>>(waveData, samplingRate);
			case aulos::WaveShape::Quadratic: return std::make_unique<aulos::StereoVoice<aulos::QuadraticShaper>>(waveData, samplingRate);
			case aulos::WaveShape::Cubic: return std::make_unique<aulos::StereoVoice<aulos::CubicShaper>>(waveData, samplingRate);
			case aulos::WaveShape::Quintic: return std::make_unique<aulos::StereoVoice<aulos::QuinticShaper>>(waveData, samplingRate);
			case aulos::WaveShape::Cosine: return std::make_unique<aulos::StereoVoice<aulos::CosineShaper>>(waveData, samplingRate);
			}
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

	class CompositionFormat
	{
	public:
		CompositionFormat(unsigned samplingRate, unsigned channels, unsigned speed) noexcept
			: _samplingRate{ samplingRate }
			, _channels{ channels }
			, _stepSamples{ static_cast<size_t>(std::lround(static_cast<double>(samplingRate) / speed)) }
		{
			assert(channels == 1 || channels == 2);
		}

		constexpr auto blockBytes() const noexcept { return _blockBytes; }
		constexpr auto channels() const noexcept { return _channels; }
		constexpr bool isStereo() const noexcept { return _channels == 2; }
		constexpr auto samplesToSteps(size_t samples) const noexcept { return (samples + _stepSamples - 1) / _stepSamples; }
		constexpr auto samplingRate() const noexcept { return _samplingRate; }
		constexpr auto stepsToSamples(size_t steps) const noexcept { return steps * _stepSamples; }

	private:
		const unsigned _samplingRate;
		const unsigned _channels;
		const size_t _blockBytes = _channels * sizeof(float);
		const size_t _stepSamples;
	};

	struct TrackRenderer
	{
	public:
		TrackRenderer(const CompositionFormat& format, const aulos::VoiceData& voiceData, const aulos::TrackProperties& trackProperties, const std::vector<AbsoluteSound>& sounds, const std::optional<std::pair<size_t, size_t>>& loop) noexcept
			: _format{ format }
			, _waveData{ voiceData, _format.samplingRate() }
			, _acoustics{ _format.isStereo() ? aulos::CircularAcoustics{ trackProperties, _format.samplingRate() } : aulos::CircularAcoustics{} }
			, _polyphony{ trackProperties._polyphony }
			, _weight{ static_cast<float>(trackProperties._weight) }
		{
			setSounds(sounds);
			if (loop)
				setLoop(loop->first, loop->second);
			setVoices(maxPolyphony(), voiceData);
		}

		size_t render(void* buffer, size_t bufferSamples) noexcept
		{
			size_t trackOffset = 0;
			while (trackOffset < bufferSamples)
			{
				if (!_strideSamplesRemaining)
				{
					if (_nextSound == _sounds.cend())
					{
						if (_loopSound == _sounds.cend())
						{
							assert(!_loopDelay);
							trackOffset = bufferSamples;
						}
						break;
					}
					const auto chordLength = _nextSound->_chordLength;
					assert(chordLength > 0);
					assert(std::distance(_nextSound, _sounds.cend()) >= chordLength);
					const auto chordEnd = _nextSound + chordLength;
					assert(std::all_of(std::next(_nextSound), chordEnd, [](const TrackSound& sound) { return !sound._delaySteps; }));
					aulos::StaticVector<PlayingSound*, aulos::kNoteCount> newSounds;
					std::for_each(_nextSound, chordEnd, [this, &newSounds](const TrackSound& sound) {
						auto i = _playingSounds.end();
						switch (_polyphony)
						{
						case aulos::Polyphony::Chord:
							for (auto j = _playingSounds.begin(); j != _playingSounds.end(); ++j)
							{
								if (std::find(newSounds.begin(), newSounds.end(), j) != newSounds.end())
									continue;
								if (i == _playingSounds.end() || i->_note < j->_note)
									i = j;
							}
							if (i == _playingSounds.end())
							{
								assert(!_voicePool.empty());
								i = &_playingSounds.emplace_back(std::move(_voicePool.back()), sound._note);
								_voicePool.pop_back();
							}
							else
								i->_note = sound._note;
							newSounds.emplace_back(i);
							break;
						case aulos::Polyphony::Full:
							i = std::find_if(_playingSounds.begin(), _playingSounds.end(), [&sound](const PlayingSound& playingSound) { return playingSound._note == sound._note; });
							if (i == _playingSounds.end())
							{
								assert(!_voicePool.empty());
								i = &_playingSounds.emplace_back(std::move(_voicePool.back()), sound._note);
								_voicePool.pop_back();
							}
							break;
						}
						i->_voice->start(i->_note, _gain, _acoustics.stereoDelay(i->_note));
					});
					_nextSound = chordEnd;
					if (_nextSound != _sounds.cend())
						_strideSamplesRemaining = _format.stepsToSamples(_nextSound->_delaySteps);
					else if (_loopDelay > 0)
					{
						_strideSamplesRemaining = _format.stepsToSamples(_loopDelay);
						_nextSound = _loopSound;
					}
					else
						_strideSamplesRemaining = std::numeric_limits<size_t>::max();
					assert(_strideSamplesRemaining > 0);
				}
				const auto strideSamples = static_cast<unsigned>(std::min(_strideSamplesRemaining, bufferSamples - trackOffset));
				unsigned maxSamplesRendered = 0;
				for (size_t i = 0; i < _playingSounds.size();)
				{
					auto& sound = _playingSounds[i];
					const auto samplesRendered = sound._voice->render(reinterpret_cast<float*>(static_cast<std::byte*>(buffer) + trackOffset * _format.blockBytes()), strideSamples);
					maxSamplesRendered = std::max(maxSamplesRendered, samplesRendered);
					if (samplesRendered < strideSamples)
					{
						_voicePool.emplace_back(std::move(sound._voice));
						if (auto j = _playingSounds.size() - 1; j != i)
							sound = std::move(_playingSounds[j]);
						_playingSounds.pop_back();
					}
					else
						++i;
				}
				if (_strideSamplesRemaining != std::numeric_limits<size_t>::max())
				{
					_strideSamplesRemaining -= strideSamples;
					trackOffset += strideSamples;
					if (_strideSamplesRemaining > 0)
					{
						assert(trackOffset == bufferSamples);
						break;
					}
				}
				else
				{
					trackOffset += maxSamplesRendered;
					if (_playingSounds.empty())
					{
						_strideSamplesRemaining = 0;
						break;
					}
				}
			}
			return trackOffset;
		}

		void restart(float gainDivisor) noexcept
		{
			for (auto& sound : _playingSounds)
			{
				sound._voice->stop();
				_voicePool.emplace_back(std::move(sound._voice));
			}
			_playingSounds.clear();
			_nextSound = _sounds.cbegin();
			_strideSamplesRemaining = _format.stepsToSamples(_nextSound->_delaySteps);
			_gain = _weight / gainDivisor;
		}

	private:
		size_t maxPolyphony() const noexcept
		{
			assert(!_sounds.empty());
			size_t result = 0;
			switch (_polyphony)
			{
			case aulos::Polyphony::Chord: {
				for (auto chordBegin = _sounds.cbegin(); chordBegin != _sounds.cend();)
				{
					result = std::max<size_t>(result, chordBegin->_chordLength);
					chordBegin += chordBegin->_chordLength;
				}
				break;
			}
			case aulos::Polyphony::Full: {
				aulos::StaticVector<aulos::Note, aulos::kNoteCount> noteCounter;
				for (const auto& sound : _sounds)
					if (std::find(noteCounter.cbegin(), noteCounter.cend(), sound._note) == noteCounter.cend())
						noteCounter.emplace_back(sound._note);
				result = noteCounter.size();
				break;
			}
			}
			return result;
		}

		void setVoices(size_t maxVoices, const aulos::VoiceData& voiceData)
		{
			assert(_voicePool.empty() && _playingSounds.empty());
			_voicePool.reserve(maxVoices);
			while (_voicePool.size() < maxVoices)
				_voicePool.emplace_back(::createVoice(_waveData, voiceData, _format.samplingRate(), _format.isStereo()));
			_playingSounds.reserve(maxVoices);
		}

		void setSounds(const std::vector<AbsoluteSound>& sounds)
		{
			assert(_sounds.empty() && !_nextSound && !_loopSound);
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

		void setLoop(size_t loopOffset, size_t loopLength) noexcept
		{
			assert(!_sounds.empty() && _loopSound == _sounds.cbegin() && !_loopDelay);
			auto loopSoundOffset = _loopSound->_delaySteps;
			while (loopSoundOffset < loopOffset)
			{
				if (++_loopSound == _sounds.cend())
					return;
				loopSoundOffset += _loopSound->_delaySteps;
			}
			if (loopLength > 0)
			{
				const auto loopDistance = _lastSoundOffset - loopSoundOffset;
				assert(loopLength > loopDistance);
				_loopDelay = loopLength - loopDistance;
			}
		}

	private:
		struct PlayingSound
		{
			std::unique_ptr<aulos::Voice> _voice;
			aulos::Note _note = aulos::Note::C0;

			PlayingSound(std::unique_ptr<aulos::Voice>&& voice, aulos::Note note) noexcept
				: _voice{ std::move(voice) }, _note{ note } {}
		};

		struct TrackSound
		{
			uint16_t _delaySteps;
			aulos::Note _note;
			uint8_t _chordLength;

			constexpr TrackSound(uint16_t delaySteps, aulos::Note note, uint8_t chordLength) noexcept
				: _delaySteps{ delaySteps }, _note{ note }, _chordLength{ chordLength } {}
		};

		const CompositionFormat& _format;
		const aulos::WaveData _waveData;
		const aulos::CircularAcoustics _acoustics;
		const aulos::Polyphony _polyphony;
		const float _weight;
		aulos::LimitedVector<std::unique_ptr<aulos::Voice>> _voicePool;
		aulos::LimitedVector<PlayingSound> _playingSounds;
		aulos::LimitedVector<TrackSound> _sounds;
		const TrackSound* _nextSound = nullptr;
		const TrackSound* _loopSound = nullptr;
		size_t _lastSoundOffset = 0;
		size_t _loopDelay = 0;
		size_t _strideSamplesRemaining = 0;
		float _gain = 0.f;
	};

	class CompositionRenderer final : public aulos::Renderer
	{
	public:
		CompositionRenderer(const aulos::CompositionImpl& composition, unsigned samplingRate, unsigned channels, bool looping)
			: _format{ samplingRate, channels, composition._speed }
			, _gainDivisor{ static_cast<float>(composition._gainDivisor) }
			, _loopOffset{ composition._loopOffset }
			, _loopLength{ composition._loopLength }
		{
			std::vector<AbsoluteSound> sounds;
			std::vector<aulos::Note> noteCounter;
			const auto maxSoundOffset = looping ? size_t{ composition._loopOffset } + composition._loopLength - 1 : std::numeric_limits<size_t>::max();
			_tracks.reserve(std::accumulate(composition._parts.cbegin(), composition._parts.cend(), size_t{}, [](size_t count, const aulos::Part& part) { return count + part._tracks.size(); }));
			for (const auto& part : composition._parts)
			{
				if (const auto soundDuration = std::accumulate(part._voice._amplitudeEnvelope._changes.cbegin(), part._voice._amplitudeEnvelope._changes.cend(), std::chrono::milliseconds::zero(),
						[](const std::chrono::milliseconds& duration, const aulos::EnvelopeChange& change) { return duration + change._duration; });
					!soundDuration.count())
					continue;
				for (const auto& track : part._tracks)
				{
					sounds.clear();
					for (size_t fragmentOffset = 0; const auto& fragment : track._fragments)
					{
						fragmentOffset += fragment._delay;
						if (fragmentOffset > maxSoundOffset)
							break;
						sounds.erase(std::find_if(sounds.crbegin(), sounds.crend(), [fragmentOffset](const AbsoluteSound& sound) { return sound._offset < fragmentOffset; }).base(), sounds.cend());
						for (auto soundOffset = fragmentOffset; const auto& sound : track._sequences[fragment._sequence])
						{
							soundOffset += sound._delay;
							if (soundOffset > maxSoundOffset)
								break;
							sounds.emplace_back(soundOffset, sound._note);
						}
					}
					if (!sounds.empty())
						_tracks.emplace_back(_format, part._voice, track._properties, sounds, looping ? std::optional{ std::pair{ composition._loopOffset, composition._loopLength } } : std::nullopt);
				}
			}
			restart();
		}

		std::pair<size_t, size_t> loopRange() const noexcept override
		{
			return { _format.stepsToSamples(_loopOffset), _format.stepsToSamples(_loopOffset + _loopLength) };
		}

		unsigned channels() const noexcept override
		{
			return _format.channels();
		}

		size_t render(void* buffer, size_t bufferBytes) noexcept override
		{
			std::memset(buffer, 0, bufferBytes);
			const auto bufferSamples = bufferBytes / _format.blockBytes();
			size_t offset = 0;
			for (auto& track : _tracks)
				offset = std::max(offset, track.render(buffer, bufferSamples));
			return offset * _format.blockBytes();
		}

		void restart() noexcept override
		{
			for (auto& track : _tracks)
				track.restart(_gainDivisor);
		}

		unsigned samplingRate() const noexcept override
		{
			return _format.samplingRate();
		}

		size_t skipSamples(size_t samples) noexcept override
		{
			static std::array<std::byte, 65'536> skipBuffer;
			const auto bufferSamples = skipBuffer.size() / _format.blockBytes();
			size_t offset = 0;
			while (offset < samples)
			{
				const auto samplesToRender = std::min(samples - offset, bufferSamples);
				size_t samplesRendered = 0;
				for (auto& track : _tracks)
					samplesRendered = std::max(samplesRendered, track.render(skipBuffer.data(), samplesToRender));
				if (samplesRendered < samplesToRender)
					break;
				offset += samplesRendered;
			}
			return offset;
		}

	public:
		CompositionFormat _format;
		const float _gainDivisor;
		const size_t _loopOffset;
		const size_t _loopLength;
		aulos::LimitedVector<TrackRenderer> _tracks;
	};
}

namespace aulos
{
	std::unique_ptr<Renderer> Renderer::create(const Composition& composition, unsigned samplingRate, unsigned channels, bool looping)
	{
		if (samplingRate < kMinSamplingRate || samplingRate > kMaxSamplingRate || (channels != 1 && channels != 2))
			return nullptr;
		const auto& compositionImpl = static_cast<const CompositionImpl&>(composition);
		if (looping && !compositionImpl.hasLoop())
			return nullptr;
		return std::make_unique<CompositionRenderer>(static_cast<const CompositionImpl&>(composition), samplingRate, channels, looping);
	}
}
