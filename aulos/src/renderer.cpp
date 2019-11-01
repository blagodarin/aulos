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

	struct EnvelopeADSR
	{
		double _attack = 0.0;
		double _decay = 0.0;
		float _sustain = 0.f;
		double _release = 0.0;
	};

	struct EnvelopeAHDSR
	{
		double _attack = 0.0;
		double _hold = 0.0;
		double _decay = 0.0;
		float _sustain = 0.f;
		double _release = 0.0;
	};

	struct Envelope
	{
	public:
		struct Part
		{
			float _left;
			float _right;
			size_t _samples;
		};

		Envelope(const EnvelopeAHDSR& envelope, size_t samplingRate) noexcept
		{
			if (const auto attack = static_cast<size_t>(envelope._attack * samplingRate); attack > 0)
				_parts[_size++] = { 0.f, 1.f, attack };
			if (const auto hold = static_cast<size_t>(envelope._hold * samplingRate); hold > 0)
				_parts[_size++] = { 1.f, 1.f, hold };
			if (const auto decay = static_cast<size_t>(envelope._decay * samplingRate); decay > 0)
				_parts[_size++] = { 1.f, envelope._sustain, decay };
			if (const auto release = static_cast<size_t>(envelope._release * samplingRate); release > 0)
				_parts[_size++] = { envelope._sustain, 0.f, release };
		}

		Envelope(const EnvelopeADSR& envelope, size_t samplingRate) noexcept
			: Envelope{ EnvelopeAHDSR{ envelope._attack, 0.0, envelope._decay, envelope._sustain, envelope._release }, samplingRate } {}

		Envelope(const EnvelopeAR& envelope, size_t samplingRate) noexcept
			: Envelope{ EnvelopeADSR{ envelope._attack, envelope._release, 0.f, 0.0 }, samplingRate } {}

		const Part* begin() const noexcept { return _parts.data(); }
		const Part* end() const noexcept { return _parts.data() + _size; }

	private:
		size_t _size = 0;
		std::array<Part, 4> _parts{};
	};

	class EnvelopeState
	{
	public:
		EnvelopeState(const Envelope& envelope) noexcept
			: _envelope{ envelope } {}

		float advance() noexcept
		{
			assert(_current != _envelope.end());
			assert(_current->_samples > 1);
			assert(_currentOffset < _current->_samples);
			const auto progress = static_cast<float>(_currentOffset) / (_current->_samples - 1);
			_lastValue = _currentLeft * (1.f - progress) + _current->_right * progress;
			++_currentOffset;
			return _lastValue;
		}

		size_t partSamplesRemaining() const noexcept
		{
			assert(_current != _envelope.end());
			return _current->_samples - _currentOffset;
		}

		bool start() noexcept
		{
			const bool restart = _current == _envelope.end();
			_current = _envelope.begin();
			if (_current != _envelope.end())
			{
				_currentLeft = restart ? _current->_left : _lastValue;
				_lastValue = _currentLeft;
				_currentOffset = 0;
			}
			return restart;
		}

		bool update() noexcept
		{
			if (_current == _envelope.end())
				return false;
			if (_currentOffset == _current->_samples)
			{
				++_current;
				if (_current == _envelope.end())
					return false;
				_currentLeft = _current->_left;
				_currentOffset = 0;
			}
			return true;
		}

	private:
		const Envelope& _envelope;
		const Envelope::Part* _current = _envelope.end();
		float _currentLeft = 0.f;
		float _lastValue = 0.f;
		size_t _currentOffset = 0;
	};

	class SquareWave
	{
	public:
		SquareWave(const Envelope& envelope, size_t samplingRate) noexcept
			: _samplingRate{ samplingRate }, _envelopeState{ envelope } {}

		void start(double frequency, float amplitude) noexcept
		{
			if (frequency <= 0.0)
				return;
			const double halfPeriodPart = _envelopeState.start() ? 1.0 : _halfPeriodRemaining / _halfPeriodLength;
			_amplitude = std::copysign(std::clamp(amplitude, -1.f, 1.f), _amplitude);
			_halfPeriodLength = _samplingRate / (2 * frequency);
			_halfPeriodRemaining = _halfPeriodLength * halfPeriodPart;
		}

		void generate(float* buffer, size_t maxSamples) noexcept
		{
			assert(maxSamples > 0);
			while (_envelopeState.update())
			{
				if (!maxSamples)
					break;
				const auto samplesToGenerate = std::min(static_cast<size_t>(std::ceil(_halfPeriodRemaining)), std::min(maxSamples, _envelopeState.partSamplesRemaining()));
				for (size_t i = 0; i < samplesToGenerate; ++i)
					buffer[i] += _amplitude * _envelopeState.advance();
				_halfPeriodRemaining -= samplesToGenerate;
				if (_halfPeriodRemaining <= 0.0)
				{
					_amplitude = -_amplitude;
					_halfPeriodRemaining += _halfPeriodLength;
				}
				buffer += samplesToGenerate;
				maxSamples -= samplesToGenerate;
			}
		}

	private:
		const size_t _samplingRate;
		EnvelopeState _envelopeState;
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
					if (!track->_noteSamplesRemaining)
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
				: _envelope{ EnvelopeAHDSR{ 0.01, 0.04, 0.05, 0.5f, 0.9 }, samplingRate }, _voice{ _envelope, samplingRate }, _note{ note }, _end{ end } {}
		};

		const CompositionImpl& _composition;
		const unsigned _samplingRate;
		const size_t _stepSamples = static_cast<size_t>(std::lround(_samplingRate / _composition._speed));
		std::vector<std::unique_ptr<TrackState>> _tracks;
	};

	std::unique_ptr<Renderer> Renderer::create(const Composition& composition, unsigned samplingRate)
	{
		return std::make_unique<RendererImpl>(static_cast<const CompositionImpl&>(composition), samplingRate);
	}
}
