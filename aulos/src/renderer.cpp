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

	class SquareWave
	{
	public:
		constexpr SquareWave(size_t samplingRate, double duration) noexcept
			: _samplingRate{ samplingRate }, _maxSamples{ static_cast<size_t>(samplingRate * duration) } {}

		void start(double frequency, float amplitude) noexcept
		{
			if (frequency <= 0.0)
			{
				_samplesToSilence = 0;
				return;
			}
			const double halfPeriodPart = _samplesToSilence > 0 ? _halfPeriodRemaining / _halfPeriodLength : 1.0;
			_amplitude = std::copysign(std::clamp(amplitude, -1.f, 1.f), _amplitude);
			_halfPeriodLength = _samplingRate / (2 * frequency);
			_halfPeriodRemaining = _halfPeriodLength * halfPeriodPart;
			_samplesToSilence = _maxSamples;
		}

		void generate(float* buffer, size_t maxSamples) noexcept
		{
			for (auto totalSamples = std::min(maxSamples, _samplesToSilence); totalSamples > 0;)
			{
				const auto partSamples = std::min(static_cast<size_t>(std::ceil(_halfPeriodRemaining)), totalSamples);
				for (size_t i = 0; i < partSamples; ++i)
					buffer[i] += _amplitude * (static_cast<float>(_samplesToSilence - (partSamples - i)) / _samplingRate);
				_halfPeriodRemaining -= partSamples;
				if (_halfPeriodRemaining <= 0.0)
				{
					_amplitude = -_amplitude;
					_halfPeriodRemaining += _halfPeriodLength;
				}
				_samplesToSilence -= partSamples;
				buffer += partSamples;
				totalSamples -= partSamples;
			}
		}

	private:
		const size_t _samplingRate;
		const size_t _maxSamples;
		double _frequency = 0.0;
		float _amplitude = 0.f;
		double _halfPeriodLength = 0.0;
		double _halfPeriodRemaining = 0.0;
		size_t _samplesToSilence = 0;
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
			SquareWave _voice;
			std::vector<NoteInfo>::const_iterator _note;
			const std::vector<NoteInfo>::const_iterator _end;
			bool _noteStarted = false;
			size_t _noteSamplesRemaining = 0;

			TrackState(unsigned samplingRate, const std::vector<NoteInfo>::const_iterator& note, const std::vector<NoteInfo>::const_iterator& end)
				: _voice{ samplingRate, 1.0 }, _note{ note }, _end{ end } {}
		};

		const CompositionImpl& _composition;
		const unsigned _samplingRate;
		const size_t _stepSamples = _samplingRate / 4;
		std::vector<std::unique_ptr<TrackState>> _tracks;
	};

	std::unique_ptr<Renderer> Renderer::create(const Composition& composition, unsigned samplingRate)
	{
		return std::make_unique<RendererImpl>(static_cast<const CompositionImpl&>(composition), samplingRate);
	}
}
