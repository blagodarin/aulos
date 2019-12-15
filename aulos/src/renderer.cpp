//
// This file is part of the Aulos toolkit.
// Copyright (C) 2019 Sergei Blagodarin.
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

#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <numeric>

namespace
{
	constexpr auto kSampleSize = sizeof(float);
	constexpr auto kNoteRatio = 1.0594630943592952645618252949463;

	class NoteTable
	{
	public:
		NoteTable() noexcept
		{
			_frequencies[static_cast<size_t>(aulos::Note::A0)] = 27.5; // A0 for the standard A440 pitch (A4 = 440 Hz).
			for (auto a = static_cast<size_t>(aulos::Note::A1); a <= static_cast<size_t>(aulos::Note::A9); a += 12)
				_frequencies[a] = _frequencies[a - 12] * 2.0;
			for (size_t base = static_cast<size_t>(aulos::Note::C0); base < _frequencies.size() - 1; base += 12)
			{
				const auto a = base + static_cast<size_t>(aulos::Note::A0) - static_cast<size_t>(aulos::Note::C0);
				for (auto note = a; note > base; --note)
					_frequencies[note - 1] = _frequencies[note] / kNoteRatio;
				for (auto note = a; note < base + static_cast<size_t>(aulos::Note::B0) - static_cast<size_t>(aulos::Note::C0); ++note)
					_frequencies[note + 1] = _frequencies[note] * kNoteRatio;
			}
		}

		constexpr double operator[](aulos::Note note) const noexcept
		{
			return _frequencies[static_cast<size_t>(note)];
		}

	private:
		std::array<double, 120> _frequencies;
	};

	const NoteTable kNoteTable;

	struct SampledEnvelope
	{
	public:
		struct Point
		{
			size_t _delay;
			double _value;
		};

		SampledEnvelope(const std::vector<aulos::Point>& envelope, size_t samplingRate) noexcept
		{
			assert(envelope.size() <= _points.size());
			for (const auto& point : envelope)
			{
				assert(point._delay >= 0.f);
				_points[_size++] = { static_cast<size_t>(double{ point._delay } * samplingRate), point._value };
			}
		}

		const Point* begin() const noexcept
		{
			return _points.data();
		}

		size_t duration() const noexcept
		{
			return std::reduce(_points.begin(), _points.end(), size_t{}, [](size_t duration, const Point& point) {
				return duration + point._delay;
			});
		}

		const Point* end() const noexcept
		{
			return _points.data() + _size;
		}

	private:
		size_t _size = 0;
		std::array<Point, 5> _points{};
	};

	class AmplitudeModulator
	{
	public:
		AmplitudeModulator(const SampledEnvelope& envelope) noexcept
			: _envelope{ envelope } {}

		void start(double value) noexcept
		{
			_nextPoint = std::find_if(_envelope.begin(), _envelope.end(), [](const SampledEnvelope::Point& point) { return point._delay > 0; });
			_baseValue = _nextPoint != _envelope.begin() ? std::prev(_nextPoint)->_value : value;
			_offset = 0;
			_currentValue = _baseValue;
		}

		double advance() noexcept
		{
			assert(_nextPoint != _envelope.end());
			assert(_nextPoint->_delay > 0);
			assert(_offset < _nextPoint->_delay);
			const auto progress = static_cast<double>(_offset) / _nextPoint->_delay;
			_currentValue = _baseValue * (1.f - progress) + _nextPoint->_value * progress;
			++_offset;
			return _currentValue;
		}

		size_t duration() const noexcept
		{
			return _envelope.duration();
		}

		size_t partSamplesRemaining() const noexcept
		{
			assert(_nextPoint != _envelope.end());
			return _nextPoint->_delay - _offset;
		}

		bool stopped() const noexcept
		{
			return _nextPoint == _envelope.end();
		}

		bool update() noexcept
		{
			for (;;)
			{
				if (_nextPoint == _envelope.end())
					return false;
				assert(_offset <= _nextPoint->_delay);
				if (_offset < _nextPoint->_delay)
					return true;
				_baseValue = _nextPoint->_value;
				++_nextPoint;
				_offset = 0;
				_currentValue = _baseValue;
			}
		}

		constexpr double value() const noexcept
		{
			return _currentValue;
		}

	private:
		const SampledEnvelope& _envelope;
		double _baseValue = 0.f;
		const SampledEnvelope::Point* _nextPoint = _envelope.end();
		size_t _offset = 0;
		double _currentValue = 0.f;
	};

	class LinearModulator
	{
	public:
		LinearModulator(const SampledEnvelope& envelope) noexcept
			: _envelope{ envelope } {}

		void start(double value) noexcept
		{
			_nextPoint = std::find_if(_envelope.begin(), _envelope.end(), [](const SampledEnvelope::Point& point) { return point._delay > 0; });
			_baseValue = _nextPoint != _envelope.begin() ? std::prev(_nextPoint)->_value : value;
			_offset = 0;
			_currentValue = _baseValue;
		}

		void advance(size_t samples) noexcept
		{
			while (_nextPoint != _envelope.end())
			{
				const auto remainingSamples = _nextPoint->_delay - _offset;
				if (remainingSamples > samples)
				{
					_offset += samples;
					const auto progress = static_cast<double>(_offset) / _nextPoint->_delay;
					_currentValue = (_baseValue * (1.0 - progress) + _nextPoint->_value * progress);
					break;
				}
				samples -= remainingSamples;
				_baseValue = _nextPoint->_value;
				++_nextPoint;
				_offset = 0;
				_currentValue = _baseValue;
			}
		}

		constexpr double value() const noexcept
		{
			return _currentValue;
		}

	private:
		const SampledEnvelope& _envelope;
		double _baseValue = 1.0;
		const SampledEnvelope::Point* _nextPoint = _envelope.end();
		size_t _offset = 0;
		double _currentValue = 0.0;
	};

	class VoiceRendererImpl : public aulos::VoiceRenderer
	{
	public:
		VoiceRendererImpl(const aulos::Voice& voice, unsigned samplingRate)
			: _amplitudeEnvelope{ voice._amplitudeEnvelope, samplingRate }
			, _frequencyEnvelope{ voice._frequencyEnvelope, samplingRate }
		{
		}

		size_t duration() const noexcept override
		{
			return _amplitudeModulator.duration() * kSampleSize;
		}

	protected:
		SampledEnvelope _amplitudeEnvelope;
		AmplitudeModulator _amplitudeModulator{ _amplitudeEnvelope };
		SampledEnvelope _frequencyEnvelope;
		LinearModulator _frequencyModulator{ _frequencyEnvelope };
	};

	class TwoPartWaveBase : public VoiceRendererImpl
	{
	public:
		TwoPartWaveBase(const aulos::Voice& voice, unsigned samplingRate) noexcept
			: VoiceRendererImpl{ voice, samplingRate }
			, _asymmetryEnvelope{ voice._asymmetryEnvelope, samplingRate }
			, _oscillation{ voice._oscillation }
			, _samplingRate{ static_cast<double>(samplingRate) }
		{
		}

		void start(aulos::Note note, float amplitude) noexcept override
		{
			const auto clampedAmplitude = std::clamp(amplitude, -1.f, 1.f);
			double partRemaining;
			if (_amplitudeModulator.stopped())
			{
				_amplitudeModulator.start(0.f);
				_amplitude = clampedAmplitude;
				_partIndex = 0;
				partRemaining = 1.0;
			}
			else
			{
				_amplitudeModulator.start(_amplitudeModulator.value());
				_amplitude = std::copysign(clampedAmplitude, _amplitude);
				partRemaining = _partSamplesRemaining / _partLength;
			}
			_frequencyModulator.start(1.0);
			_asymmetryModulator.start(0.0);
			_frequency = kNoteTable[note];
			updatePeriodParts();
			_partSamplesRemaining = _partLength * partRemaining;
			advance(0);
		}

	protected:
		void advance(size_t samples) noexcept
		{
			_frequencyModulator.advance(samples);
			_asymmetryModulator.advance(samples);
			auto remaining = _partSamplesRemaining - samples;
			while (remaining <= 0.0)
			{
				_amplitude = -_amplitude;
				_partIndex = 1 - _partIndex;
				updatePeriodParts();
				remaining += _partLength;
			}
			_partSamplesRemaining = remaining;
		}

		void updatePeriodParts() noexcept
		{
			const auto periodSamples = _samplingRate / (_frequency * _frequencyModulator.value());
			const auto dutyCycle = _asymmetryModulator.value() * 0.5 + 0.5;
			_partLength = periodSamples * (_partIndex == 0 ? dutyCycle : 1.0 - dutyCycle);
		}

	protected:
		SampledEnvelope _asymmetryEnvelope;
		LinearModulator _asymmetryModulator{ _asymmetryEnvelope };
		const double _oscillation;
		const double _samplingRate;
		float _amplitude = 0.f;
		double _frequency = 0.0;
		size_t _partIndex = 0;
		double _partLength = 0.0;
		double _partSamplesRemaining = 0.0;
	};

	template <typename Oscillator>
	class TwoPartWave final : public TwoPartWaveBase
	{
	public:
		using TwoPartWaveBase::TwoPartWaveBase;

		size_t render(void* buffer, size_t bufferSize) noexcept override
		{
			bufferSize -= bufferSize % kSampleSize;
			size_t offset = 0;
			while (offset < bufferSize && _amplitudeModulator.update())
			{
				const auto samplesToGenerate = std::min(static_cast<size_t>(std::ceil(_partSamplesRemaining)), std::min((bufferSize - offset) / kSampleSize, _amplitudeModulator.partSamplesRemaining()));
				Oscillator oscillator{ _partSamplesRemaining, _partLength, _oscillation };
				const auto base = reinterpret_cast<float*>(static_cast<std::byte*>(buffer) + offset);
				for (size_t i = 0; i < samplesToGenerate; ++i)
					base[i] += static_cast<float>(_amplitude * oscillator() * _amplitudeModulator.advance());
				advance(samplesToGenerate);
				offset += samplesToGenerate * kSampleSize;
			}
			return offset;
		}
	};

	struct LinearOscillator
	{
		const double _increment;
		double _value;

		constexpr LinearOscillator(double remainingSamples, double totalSamples, double oscillation) noexcept
			: _increment{ 2.0 * oscillation / totalSamples }, _value{ 1.0 - _increment * (remainingSamples - 1.0) } {}

		constexpr double operator()() noexcept
		{
			return _value += _increment;
		}
	};

	class RendererImpl final : public aulos::Renderer
	{
	public:
		RendererImpl(const aulos::CompositionImpl& composition, unsigned samplingRate)
			: _samplingRate{ samplingRate }
			, _stepBytes{ static_cast<size_t>(std::lround(_samplingRate / composition._speed)) * kSampleSize }
		{
			const auto totalWeight = static_cast<float>(std::reduce(composition._tracks.cbegin(), composition._tracks.cend(), 0u, [](unsigned weight, const aulos::Track& track) { return weight + track._weight; }));
			_tracks.reserve(composition._tracks.size());
			for (const auto& track : composition._tracks)
				_tracks.emplace_back(std::make_unique<TrackState>(composition._voices[track._voice], track._weight / totalWeight, samplingRate));
			size_t fragmentOffset = 0;
			for (const auto& fragment : composition._fragments)
			{
				fragmentOffset += fragment._delay;
				auto& trackSounds = _tracks[fragment._track]->_sounds;
				auto lastSoundOffset = std::reduce(trackSounds.cbegin(), trackSounds.cend(), size_t{}, [](size_t offset, const aulos::Sound& sound) { return offset + sound._delay; });
				while (!trackSounds.empty() && lastSoundOffset >= fragmentOffset)
				{
					lastSoundOffset -= trackSounds.back()._delay;
					trackSounds.pop_back();
				}
				if (const auto& sequence = composition._sequences[fragment._sequence]; !sequence.empty())
				{
					trackSounds.reserve(trackSounds.size() + sequence.size());
					trackSounds.emplace_back(fragmentOffset - lastSoundOffset + sequence.front()._delay, sequence.front()._note);
					std::copy(std::next(sequence.begin()), sequence.cend(), std::back_inserter(trackSounds));
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

		size_t render(void* buffer, size_t bufferBytes) noexcept override
		{
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
							: track->_voice->duration();
						assert(track->_soundBytesRemaining % kSampleSize == 0);
						track->_voice->start(track->_sounds[track->_soundIndex]._note, track->_normalizedWeight);
					}
					const auto bytesToGenerate = std::min(track->_soundBytesRemaining, bufferBytes - trackOffset);
					if (!bytesToGenerate)
						break;
					track->_voice->render(static_cast<std::byte*>(buffer) + trackOffset, bytesToGenerate);
					track->_soundBytesRemaining -= bytesToGenerate;
					trackOffset += bytesToGenerate;
					if (!track->_soundBytesRemaining)
						++track->_soundIndex;
				}
				offset = std::max(offset, trackOffset);
			}
			return offset;
		}

	public:
		struct TrackState
		{
			std::unique_ptr<aulos::VoiceRenderer> _voice;
			const float _normalizedWeight;
			std::vector<aulos::Sound> _sounds;
			size_t _soundIndex = 0;
			size_t _soundBytesRemaining = 0;

			TrackState(const aulos::Voice& voice, float normalizedWeight, unsigned samplingRate)
				: _voice{ aulos::VoiceRenderer::create(voice, samplingRate) }
				, _normalizedWeight{ normalizedWeight }
			{
			}
		};

		const unsigned _samplingRate;
		const size_t _stepBytes;
		std::vector<std::unique_ptr<TrackState>> _tracks;
	};
}

namespace aulos
{
	std::unique_ptr<VoiceRenderer> VoiceRenderer::create(const Voice& voice, unsigned samplingRate)
	{
		switch (voice._wave)
		{
		case Wave::Linear: return std::make_unique<TwoPartWave<LinearOscillator>>(voice, samplingRate);
		}
		return {};
	}

	std::unique_ptr<Renderer> Renderer::create(const Composition& composition, unsigned samplingRate)
	{
		return std::make_unique<RendererImpl>(static_cast<const CompositionImpl&>(composition), samplingRate);
	}
}
