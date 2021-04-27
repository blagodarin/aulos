// This file is part of the Aulos toolkit.
// Copyright (C) Sergei Blagodarin.
// SPDX-License-Identifier: Apache-2.0

#include <aulos/renderer.hpp>

#include <aulos/format.hpp>
#include "acoustics.hpp"
#include "composition.hpp"
#include "tables.hpp"
#include "voice.hpp"

#include <primal/rigid_vector.hpp>
#include <primal/static_vector.hpp>

#include <algorithm>
#include <cstring>
#include <functional>
#include <numeric>

namespace
{
	std::unique_ptr<aulos::Voice> createVoice(const aulos::WaveData& waveData, const aulos::VoiceData& voiceData, const aulos::AudioFormat& format)
	{
		switch (format.channelLayout())
		{
		case aulos::ChannelLayout::Mono:
			switch (voiceData._waveShape)
			{
			case aulos::WaveShape::Linear: return std::make_unique<aulos::MonoVoice<aulos::LinearShaper>>(waveData, format.samplingRate());
			case aulos::WaveShape::Quadratic: return std::make_unique<aulos::MonoVoice<aulos::QuadraticShaper>>(waveData, format.samplingRate());
			case aulos::WaveShape::Cubic: return std::make_unique<aulos::MonoVoice<aulos::CubicShaper>>(waveData, format.samplingRate());
			case aulos::WaveShape::Quintic: return std::make_unique<aulos::MonoVoice<aulos::QuinticShaper>>(waveData, format.samplingRate());
			case aulos::WaveShape::Cosine: return std::make_unique<aulos::MonoVoice<aulos::CosineShaper>>(waveData, format.samplingRate());
			}
			break;
		case aulos::ChannelLayout::Stereo:
			switch (voiceData._waveShape)
			{
			case aulos::WaveShape::Linear: return std::make_unique<aulos::StereoVoice<aulos::LinearShaper>>(waveData, format.samplingRate());
			case aulos::WaveShape::Quadratic: return std::make_unique<aulos::StereoVoice<aulos::QuadraticShaper>>(waveData, format.samplingRate());
			case aulos::WaveShape::Cubic: return std::make_unique<aulos::StereoVoice<aulos::CubicShaper>>(waveData, format.samplingRate());
			case aulos::WaveShape::Quintic: return std::make_unique<aulos::StereoVoice<aulos::QuinticShaper>>(waveData, format.samplingRate());
			case aulos::WaveShape::Cosine: return std::make_unique<aulos::StereoVoice<aulos::CosineShaper>>(waveData, format.samplingRate());
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

	struct TrackRenderer
	{
	public:
		TrackRenderer(const aulos::AudioFormat& format, size_t stepFrames, const aulos::VoiceData& voiceData, const aulos::TrackProperties& trackProperties, const std::vector<AbsoluteSound>& sounds, size_t loopOffset, size_t loopLength) noexcept
			: _format{ format }
			, _stepFrames{ stepFrames }
			, _waveData{ voiceData, format.samplingRate() }
			, _acoustics{ format.channelLayout() == aulos::ChannelLayout::Mono ? aulos::CircularAcoustics{} : aulos::CircularAcoustics{ trackProperties, format.samplingRate() } }
			, _polyphony{ trackProperties._polyphony }
			, _weight{ static_cast<float>(trackProperties._weight) }
		{
			setSounds(sounds);
			if (loopLength > 0)
				setLoop(loopOffset, loopLength);
			setVoices(maxPolyphony(), voiceData);
		}

		size_t render(float* buffer, size_t maxFrames) noexcept
		{
			size_t trackOffset = 0;
			while (trackOffset < maxFrames)
			{
				if (!_strideFramesRemaining)
				{
					if (_nextSound == _sounds.cend())
					{
						if (_loopSound == _sounds.cend())
						{
							assert(!_loopDelay);
							trackOffset = maxFrames;
						}
						break;
					}
					const auto chordLength = _nextSound->_chordLength;
					assert(chordLength > 0);
					assert(std::distance(_nextSound, _sounds.cend()) >= chordLength);
					const auto chordEnd = _nextSound + chordLength;
					assert(std::all_of(std::next(_nextSound), chordEnd, [](const TrackSound& sound) { return !sound._delaySteps; }));
					primal::StaticVector<PlayingSound*, aulos::kNoteCount> newSounds;
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
						i->_voice->start(aulos::kNoteFrequencies[i->_note], _gain, _acoustics.stereoDelay(i->_note));
					});
					_nextSound = chordEnd;
					if (_nextSound != _sounds.cend())
						_strideFramesRemaining = _nextSound->_delaySteps * _stepFrames;
					else if (_loopDelay > 0)
					{
						_strideFramesRemaining = _loopDelay * _stepFrames;
						_nextSound = _loopSound;
					}
					else
						_strideFramesRemaining = std::numeric_limits<size_t>::max();
					assert(_strideFramesRemaining > 0);
				}
				const auto framesToRender = static_cast<unsigned>(std::min(_strideFramesRemaining, maxFrames - trackOffset));
				unsigned maxFramesRendered = 0;
				for (size_t i = 0; i < _playingSounds.size();)
				{
					auto& sound = _playingSounds[i];
					const auto framesRendered = sound._voice->render(buffer + trackOffset * _format.channelCount(), framesToRender);
					maxFramesRendered = std::max(maxFramesRendered, framesRendered);
					if (framesRendered < framesToRender)
					{
						_voicePool.emplace_back(std::move(sound._voice));
						if (auto j = _playingSounds.size() - 1; j != i)
							sound = std::move(_playingSounds[j]);
						_playingSounds.pop_back();
					}
					else
						++i;
				}
				if (_strideFramesRemaining != std::numeric_limits<size_t>::max())
				{
					// If the composition hasn't ended, we should advance by the number of frames we wanted to render,
					// not the number of frames actually rendered, to preserve silent parts of the composition.
					_strideFramesRemaining -= framesToRender;
					trackOffset += framesToRender;
					if (_strideFramesRemaining > 0)
					{
						assert(trackOffset == maxFrames);
						break;
					}
				}
				else
				{
					trackOffset += maxFramesRendered;
					if (_playingSounds.empty())
					{
						_strideFramesRemaining = 0;
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
			_strideFramesRemaining = _nextSound->_delaySteps * _stepFrames;
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
				primal::StaticVector<aulos::Note, aulos::kNoteCount> noteCounter;
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
				_voicePool.emplace_back(::createVoice(_waveData, voiceData, _format));
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
			auto loopSoundOffset = size_t{ _loopSound->_delaySteps };
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

		const aulos::AudioFormat _format;
		const size_t _stepFrames;
		const aulos::WaveData _waveData;
		const aulos::CircularAcoustics _acoustics;
		const aulos::Polyphony _polyphony;
		const float _weight;
		primal::RigidVector<std::unique_ptr<aulos::Voice>> _voicePool;
		primal::RigidVector<PlayingSound> _playingSounds;
		primal::RigidVector<TrackSound> _sounds;
		const TrackSound* _nextSound = nullptr;
		const TrackSound* _loopSound = nullptr;
		size_t _lastSoundOffset = 0;
		size_t _loopDelay = 0;
		size_t _strideFramesRemaining = 0;
		float _gain = 0.f;
	};

	class CompositionRenderer final : public aulos::Renderer
	{
	public:
		CompositionRenderer(const aulos::CompositionImpl& composition, const aulos::AudioFormat& format, bool looping)
			: _format{ format }
			, _stepFrames{ static_cast<size_t>(std::lround(static_cast<double>(format.samplingRate()) / composition._speed)) }
			, _gainDivisor{ static_cast<float>(composition._gainDivisor) }
			, _looping{ looping }
		{
			const auto loopStepCount = _looping ? size_t{ composition._loopLength } : 0;
			const auto loopStepOffset = loopStepCount > 0 ? size_t{ composition._loopOffset } : 0;
			std::vector<AbsoluteSound> sounds;
			const auto maxSoundStep = loopStepCount > 0 ? loopStepOffset + loopStepCount - 1 : std::numeric_limits<size_t>::max();
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
					for (size_t fragmentStep = 0; const auto& fragment : track._fragments)
					{
						fragmentStep += fragment._delay;
						if (fragmentStep > maxSoundStep)
							break;
						sounds.erase(std::find_if(sounds.crbegin(), sounds.crend(), [fragmentStep](const AbsoluteSound& sound) { return sound._offset < fragmentStep; }).base(), sounds.cend());
						for (auto soundStep = fragmentStep; const auto& sound : track._sequences[fragment._sequence])
						{
							soundStep += sound._delay;
							if (soundStep > maxSoundStep)
								break;
							sounds.emplace_back(soundStep, sound._note);
						}
					}
					if (!sounds.empty())
						_tracks.emplace_back(_format, _stepFrames, part._voice, track._properties, sounds, loopStepOffset, loopStepCount);
				}
			}
			_loopOffset = loopStepOffset * _stepFrames;
			_loopLength = loopStepCount * _stepFrames;
			restart();
		}

		aulos::AudioFormat format() const noexcept override
		{
			return _format;
		}

		size_t currentOffset() const noexcept override
		{
			return _currentOffset;
		}

		size_t loopOffset() const noexcept override
		{
			return _loopOffset;
		}

		size_t render(float* buffer, size_t maxFrames) noexcept override
		{
			std::memset(buffer, 0, maxFrames * _format.bytesPerFrame());
			size_t result = 0;
			while (result < maxFrames)
			{
				const auto [framesRendered, stopped] = renderPart(buffer + result * _format.channelCount(), maxFrames - result);
				result += framesRendered;
				if (stopped)
					break;
			}
			return result;
		}

		void restart() noexcept override
		{
			for (auto& track : _tracks)
				track.restart(_gainDivisor);
			_currentOffset = 0;
		}

		size_t skipFrames(size_t maxFrames) noexcept override
		{
			static std::array<std::byte, 65'536> skipBuffer;
			size_t result = 0;
			while (result < maxFrames)
			{
				const auto [framesRendered, stopped] = renderPart(reinterpret_cast<float*>(skipBuffer.data()), std::min(maxFrames - result, skipBuffer.size() / _format.bytesPerFrame()));
				result += framesRendered;
				if (stopped)
					break;
			}
			return result;
		}

	private:
		std::pair<size_t, bool> renderPart(float* buffer, size_t maxFrames) noexcept
		{
			size_t framesRendered = 0;
			for (auto& track : _tracks)
				framesRendered = std::max(framesRendered, track.render(buffer, maxFrames));
			_currentOffset += framesRendered;
			if (_looping && _loopLength > 0)
				while (_currentOffset >= _loopOffset + _loopLength)
					_currentOffset -= _loopLength;
			if (framesRendered < maxFrames)
			{
				if (!_looping)
					return { framesRendered, true };
				const auto framesToSkip = _loopLength > 0
					? _loopOffset + _loopLength - _currentOffset  // The composition is empty, but has a loop.
					: _stepFrames - _currentOffset % _stepFrames; // The composition has no loop but is requested to be played in a loop.
				const auto framesSkipped = std::min(maxFrames - framesRendered, framesToSkip);
				framesRendered += framesSkipped;
				_currentOffset += framesSkipped;
				if (framesSkipped == framesToSkip)
				{
					if (_loopLength > 0)
					{
						assert(_tracks.empty());
						_currentOffset = _loopOffset;
					}
					else
						restart();
				}
			}
			return { framesRendered, false };
		}

	public:
		const aulos::AudioFormat _format;
		const size_t _stepFrames;
		const float _gainDivisor;
		const bool _looping;
		primal::RigidVector<TrackRenderer> _tracks;
		size_t _currentOffset = 0;
		size_t _loopOffset = 0;
		size_t _loopLength = 0;
	};
}

namespace aulos
{
	std::unique_ptr<Renderer> Renderer::create(const Composition& composition, const AudioFormat& format, bool looping)
	{
		if (format.samplingRate() < kMinSamplingRate || format.samplingRate() > kMaxSamplingRate)
			return nullptr;
		return std::make_unique<CompositionRenderer>(static_cast<const CompositionImpl&>(composition), format, looping);
	}
}
