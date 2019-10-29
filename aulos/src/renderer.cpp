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
			for (size_t base = 0; base < _frequencies.size() - 1; base += 12)
			{
				const auto a = base + static_cast<size_t>(aulos::Note::A0) - static_cast<size_t>(aulos::Note::C0);
				for (auto note = a; note > base; --note)
					_frequencies[note - 1] = _frequencies[note] / kNoteRatio;
				for (auto note = a; note < base + static_cast<size_t>(aulos::Note::B0) - static_cast<size_t>(aulos::Note::C0); ++note)
					_frequencies[note + 1] = _frequencies[note] * kNoteRatio;
			}
			_frequencies[static_cast<size_t>(aulos::Note::Silence)] = 0.0;
		}

		constexpr double operator[](aulos::Note note) const noexcept
		{
			return _frequencies[static_cast<size_t>(note)];
		}

	private:
		std::array<double, 121> _frequencies;
	};

	const NoteTable kNoteTable;

	// Attack-Release envelope.
	struct EnvelopeAR
	{
		double _attack = 0.0;  // Seconds from zero to full amplitude.
		double _release = 0.0; // Seconds from full amplitude to zero.
	};

	struct Envelope
	{
		struct Part
		{
			float _left = 0.f;
			float _right = 0.f;
			size_t _samples = 0;

			constexpr Part(float left, float right, size_t samples) noexcept
				: _left{ left }, _right{ right }, _samples{ samples } {}
		};

		std::vector<Part> _parts;

		Envelope(const EnvelopeAR& envelope, size_t samplingRate)
		{
			if (const auto attack = static_cast<size_t>(envelope._attack * samplingRate); attack > 0)
				_parts.emplace_back(0.f, 1.f, attack);
			if (const auto release = static_cast<size_t>(envelope._release * samplingRate); release > 0)
				_parts.emplace_back(1.f, 0.f, release);
		}
	};

	class SquareWave
	{
	public:
		SquareWave(const Envelope& envelope, size_t samplingRate) noexcept
			: _samplingRate{ samplingRate }, _envelope{ envelope } {}

		void start(double frequency, float amplitude) noexcept
		{
			if (frequency <= 0.0)
			{
				_current = _envelope._parts.cend();
				return;
			}
			const double halfPeriodPart = _current != _envelope._parts.cend() ? _halfPeriodRemaining / _halfPeriodLength : 1.0;
			_amplitude = std::copysign(std::clamp(amplitude, -1.f, 1.f), _amplitude);
			_halfPeriodLength = _samplingRate / (2 * frequency);
			_halfPeriodRemaining = _halfPeriodLength * halfPeriodPart;
			_current = _envelope._parts.cbegin();
			_samplesToSwitch = _current->_samples;
		}

		void generate(float* buffer, size_t maxSamples) noexcept
		{
			assert(maxSamples > 0);
			while (_current != _envelope._parts.cend())
			{
				if (!_samplesToSwitch)
				{
					++_current;
					if (_current != _envelope._parts.cend())
						_samplesToSwitch = _current->_samples;
					continue;
				}
				if (!maxSamples)
					break;
				const auto halfPeriodSamples = std::min(static_cast<size_t>(std::ceil(_halfPeriodRemaining)), std::min(maxSamples, _samplesToSwitch));
				for (size_t i = 0; i < halfPeriodSamples; ++i)
				{
					const auto progress = static_cast<float>(_samplesToSwitch - (halfPeriodSamples - i)) / _samplingRate;
					buffer[i] += _amplitude * (_current->_left * progress + _current->_right * (1.f - progress));
				}
				_halfPeriodRemaining -= halfPeriodSamples;
				if (_halfPeriodRemaining <= 0.0)
				{
					_amplitude = -_amplitude;
					_halfPeriodRemaining += _halfPeriodLength;
				}
				_samplesToSwitch -= halfPeriodSamples;
				buffer += halfPeriodSamples;
				maxSamples -= halfPeriodSamples;
			}
		}

	private:
		const size_t _samplingRate;
		const Envelope& _envelope;
		std::vector<Envelope::Part>::const_iterator _current = _envelope._parts.cend();
		size_t _samplesToSwitch = 0;
		double _frequency = 0.0;
		float _amplitude = 0.f;
		double _halfPeriodLength = 0.0;
		double _halfPeriodRemaining = 0.0;
	};
}

namespace aulos
{
	class RendererImpl final : public Renderer
	{
	public:
		RendererImpl(const CompositionImpl& composition, unsigned samplingRate)
			: _composition{ composition }
			, _samplingRate{ samplingRate }
		{
			_tracks.reserve(_composition._tracks.size());
			for (const auto& track : _composition._tracks)
				_tracks.emplace_back(std::make_unique<RendererImpl::TrackState>(samplingRate, track.cbegin(), track.cend()));
		}

		size_t render(void* buffer, size_t bufferBytes) noexcept override
		{
			if (_tracks.empty())
				return 0;
			std::memset(buffer, 0, bufferBytes);
			size_t samplesRendered = 0;
			for (auto& track : _tracks)
			{
				size_t trackSamplesRendered = 0;
				while (track->_note != track->_end)
				{
					if (!track->_noteStarted)
					{
						track->_noteStarted = true;
						track->_noteSamplesRemaining = _stepSamples * track->_note->_duration;
						track->_voice.start(kNoteTable[track->_note->_note], .25f);
					}
					const auto samplesToGenerate = bufferBytes / kSampleSize - trackSamplesRendered;
					if (!samplesToGenerate)
						break;
					const auto samplesGenerated = std::min(track->_noteSamplesRemaining, samplesToGenerate);
					track->_voice.generate(static_cast<float*>(buffer) + trackSamplesRendered, samplesGenerated);
					track->_noteSamplesRemaining -= samplesGenerated;
					trackSamplesRendered += samplesGenerated;
					if (samplesGenerated < samplesToGenerate)
					{
						++track->_note;
						track->_noteStarted = false;
					}
				}
				samplesRendered = std::max(samplesRendered, trackSamplesRendered);
			}
			return samplesRendered * kSampleSize;
		}

	public:
		struct TrackState
		{
			Envelope _envelope;
			SquareWave _voice;
			std::vector<NoteInfo>::const_iterator _note;
			const std::vector<NoteInfo>::const_iterator _end;
			bool _noteStarted = false;
			size_t _noteSamplesRemaining = 0;

			TrackState(unsigned samplingRate, const std::vector<NoteInfo>::const_iterator& note, const std::vector<NoteInfo>::const_iterator& end)
				: _envelope{ { 0.25, 0.75 }, samplingRate }, _voice{ _envelope, samplingRate }, _note{ note }, _end{ end } {}
		};

		const CompositionImpl& _composition;
		const unsigned _samplingRate;
		const size_t _stepSamples = _samplingRate * 15ul / _composition._tempo;
		std::vector<std::unique_ptr<TrackState>> _tracks;
	};

	std::unique_ptr<Renderer> Renderer::create(const Composition& composition, unsigned samplingRate)
	{
		return std::make_unique<RendererImpl>(static_cast<const CompositionImpl&>(composition), samplingRate);
	}
}
