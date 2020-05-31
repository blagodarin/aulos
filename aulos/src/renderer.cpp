// This file is part of the Aulos toolkit.
// Copyright (C) Sergei Blagodarin.
// SPDX-License-Identifier: Apache-2.0

#include "composition.hpp"
#include "utils.hpp"
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
	std::unique_ptr<aulos::Voice> createVoice(const aulos::WaveData& waveData, const aulos::VoiceData& voiceData, unsigned samplingRate, bool isStereo)
	{
		if (!isStereo)
		{
			switch (voiceData._waveShape)
			{
			case aulos::WaveShape::Linear: return std::make_unique<aulos::MonoVoice<aulos::LinearShaper>>(waveData, voiceData, samplingRate);
			case aulos::WaveShape::SmoothQuadratic: return std::make_unique<aulos::MonoVoice<aulos::SmoothQuadraticShaper>>(waveData, voiceData, samplingRate);
			case aulos::WaveShape::SharpQuadratic: return std::make_unique<aulos::MonoVoice<aulos::SharpQuadraticShaper>>(waveData, voiceData, samplingRate);
			case aulos::WaveShape::SmoothCubic: return std::make_unique<aulos::MonoVoice<aulos::SmoothCubicShaper>>(waveData, voiceData, samplingRate);
			case aulos::WaveShape::Quintic: return std::make_unique<aulos::MonoVoice<aulos::QuinticShaper>>(waveData, voiceData, samplingRate);
			case aulos::WaveShape::Cosine: return std::make_unique<aulos::MonoVoice<aulos::CosineShaper>>(waveData, voiceData, samplingRate);
			}
		}
		else
		{
			switch (voiceData._waveShape)
			{
			case aulos::WaveShape::Linear: return std::make_unique<aulos::StereoVoice<aulos::LinearShaper>>(waveData, voiceData, samplingRate);
			case aulos::WaveShape::SmoothQuadratic: return std::make_unique<aulos::StereoVoice<aulos::SmoothQuadraticShaper>>(waveData, voiceData, samplingRate);
			case aulos::WaveShape::SharpQuadratic: return std::make_unique<aulos::StereoVoice<aulos::SharpQuadraticShaper>>(waveData, voiceData, samplingRate);
			case aulos::WaveShape::SmoothCubic: return std::make_unique<aulos::StereoVoice<aulos::SmoothCubicShaper>>(waveData, voiceData, samplingRate);
			case aulos::WaveShape::Quintic: return std::make_unique<aulos::StereoVoice<aulos::QuinticShaper>>(waveData, voiceData, samplingRate);
			case aulos::WaveShape::Cosine: return std::make_unique<aulos::StereoVoice<aulos::CosineShaper>>(waveData, voiceData, samplingRate);
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
		constexpr auto stepsToBytes(size_t steps) const noexcept { return steps * _stepBytes; }
		constexpr auto stepsToSamples(size_t steps) const noexcept { return steps * _stepSamples; }

	private:
		const unsigned _samplingRate;
		const unsigned _channels;
		const size_t _blockBytes = _channels * sizeof(float);
		const size_t _stepSamples;
		const size_t _stepBytes = _stepSamples * _blockBytes;
	};

	struct TrackRenderer
	{
	public:
		TrackRenderer(const CompositionFormat& format, const aulos::VoiceData& voiceData) noexcept
			: _format{ format }
			, _waveData{ voiceData, _format.samplingRate(), _format.isStereo() }
			, _polyphony{ voiceData._polyphony }
		{
			assert(_soundSamples > 0);
		}

		size_t maxPolyphony(std::vector<aulos::Note>& noteCounter) const noexcept
		{
			assert(!_sounds.empty());
			size_t maxPolyphony = 0;
			switch (_polyphony)
			{
			case aulos::Polyphony::Chord:
				for (auto chordBegin = _sounds.cbegin(); chordBegin != _sounds.cend();)
				{
					maxPolyphony = std::max<size_t>(maxPolyphony, chordBegin->_chordLength);
					chordBegin += chordBegin->_chordLength;
				}
				break;
			case aulos::Polyphony::Full: {
				const auto soundSteps = _format.samplesToSteps(_soundSamples);
				for (auto i = _sounds.cbegin(); i != _sounds.cend(); ++i)
				{
					noteCounter.clear();
					noteCounter.emplace_back(i->_note);
					size_t currentDelay = 0;
					for (auto j = std::next(i); j != _sounds.cend(); ++j)
					{
						currentDelay += j->_delaySteps;
						if (currentDelay >= soundSteps)
							break;
						if (std::none_of(noteCounter.cbegin(), noteCounter.cend(), [j](aulos::Note note) { return note == j->_note; }))
							noteCounter.emplace_back(j->_note);
					}
					maxPolyphony = std::max(maxPolyphony, noteCounter.size());
				}
				// TODO: Take looping into account.
				break;
			}
			}
			return maxPolyphony;
		}

		void setVoices(size_t maxVoices, const aulos::VoiceData& voiceData)
		{
			assert(_voicePool.empty() && _activeSounds.empty());
			_voicePool.reserve(maxVoices);
			while (_voicePool.size() < maxVoices)
				_voicePool.emplace_back(::createVoice(_waveData, voiceData, _format.samplingRate(), _format.isStereo()));
			_activeSounds.reserve(maxVoices);
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
			const auto loopDistance = _lastSoundOffset - loopSoundOffset;
			assert(loopLength > loopDistance);
			_loopDelay = loopLength - loopDistance;
		}

		void setWeight(float weight) noexcept
		{
			assert(_weight == 0);
			assert(weight > 0);
			_weight = weight;
		}

		size_t render(void* buffer, size_t bufferBytes) noexcept
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
								i = &_activeSounds.emplace_back(std::move(_voicePool.back()), sound._note);
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
								i = &_activeSounds.emplace_back(std::move(_voicePool.back()), sound._note);
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
							_strideBytesRemaining = _format.stepsToBytes(_loopDelay);
							_nextSound = _loopSound;
						}
						else
							_strideBytesRemaining = _soundBytes;
					}
					else
						_strideBytesRemaining = _format.stepsToBytes(_nextSound->_delaySteps);
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

		void restart(float gain) noexcept
		{
			for (auto& activeVoice : _activeSounds)
			{
				activeVoice._voice->stop();
				_voicePool.emplace_back(std::move(activeVoice._voice));
			}
			_activeSounds.clear();
			_nextSound = _sounds.cbegin();
			_strideBytesRemaining = _format.stepsToBytes(_nextSound->_delaySteps);
			_gain = _weight * gain;
		}

		size_t totalSamples() const noexcept
		{
			assert(!_sounds.empty());
			return _format.stepsToSamples(_lastSoundOffset) + _soundSamples;
		}

	private:
		struct ActiveSound
		{
			std::unique_ptr<aulos::Voice> _voice;
			size_t _bytesRemaining = 0;
			aulos::Note _note = aulos::Note::C0;

			ActiveSound(std::unique_ptr<aulos::Voice>&& voice, aulos::Note note) noexcept
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
		const aulos::Polyphony _polyphony;
		const size_t _soundSamples = _waveData.totalSamples();
		const size_t _soundBytes = _soundSamples * _format.blockBytes();
		float _weight = 0;
		aulos::StaticVector<std::unique_ptr<aulos::Voice>> _voicePool;
		aulos::StaticVector<ActiveSound> _activeSounds;
		aulos::StaticVector<TrackSound> _sounds;
		const TrackSound* _nextSound = nullptr;
		const TrackSound* _loopSound = nullptr;
		size_t _lastSoundOffset = 0;
		size_t _loopDelay = 0;
		size_t _strideBytesRemaining = 0;
		float _gain = 0.f;
	};

	class CompositionRenderer final : public aulos::Renderer
	{
	public:
		CompositionRenderer(const aulos::CompositionImpl& composition, unsigned samplingRate, unsigned channels, bool looping)
			: _format{ samplingRate, channels, composition._speed }
			, _loopOffset{ composition._loopLength > 0 ? composition._loopOffset : 0 }
		{
			std::vector<AbsoluteSound> sounds;
			std::vector<aulos::Note> noteCounter;
			const auto loopEnd = looping && composition._loopLength > 0 ? size_t{ composition._loopOffset } + composition._loopLength : std::numeric_limits<size_t>::max();
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
					auto& trackRenderer = _tracks.emplace_back(_format, part._voice);
					trackRenderer.setSounds(sounds);
					const auto maxPolyphony = trackRenderer.maxPolyphony(noteCounter);
					trackRenderer.setWeight(static_cast<float>(track._weight) / maxPolyphony);
					trackRenderer.setVoices(maxPolyphony, part._voice);
				}
			}
			_loopLength = composition._loopLength > 0 ? composition._loopLength : _format.samplesToSteps(totalSamples());
			if (looping)
				for (auto& track : _tracks)
					track.setLoop(_loopOffset, _loopLength);
			restart(1);
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
			bufferBytes -= bufferBytes % _format.blockBytes();
			if (buffer)
				std::memset(buffer, 0, bufferBytes);
			size_t offset = 0;
			for (auto& track : _tracks)
				offset = std::max(offset, track.render(buffer, bufferBytes));
			return offset;
		}

		void restart(float gain) noexcept override
		{
			assert(gain >= 1);
			for (auto& track : _tracks)
				track.restart(gain);
		}

		unsigned samplingRate() const noexcept override
		{
			return _format.samplingRate();
		}

		size_t totalSamples() const noexcept override
		{
			return std::accumulate(_tracks.cbegin(), _tracks.cend(), _format.stepsToSamples(_loopOffset + _loopLength),
				[this](size_t samples, const TrackRenderer& track) { return std::max(samples, track.totalSamples()); });
		}

	public:
		CompositionFormat _format;
		const size_t _loopOffset;
		size_t _loopLength = 0;
		aulos::StaticVector<TrackRenderer> _tracks;
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
