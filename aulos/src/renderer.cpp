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

	class Voice
	{
	public:
		constexpr Voice(size_t samplingRate, double duration) noexcept
			: _samplingRate{ samplingRate }, _maxSamples{ static_cast<size_t>(samplingRate * duration) } {}

		void generate(float* base, size_t count) noexcept
		{
			if (!_samplesToSilence)
				return;
			const auto samplesPerHalfPeriod = _samplingRate / (2 * _frequency);
			auto remainingHalfPeriod = static_cast<size_t>(_halfPeriodRemaining * samplesPerHalfPeriod);
			while (remainingHalfPeriod <= count)
			{
				const auto samples = std::min(remainingHalfPeriod, _samplesToSilence);
				for (auto i = samples; i > 0; --i)
					*base++ += _amplitude * (static_cast<float>(_samplesToSilence - i) / _samplingRate);
				count -= remainingHalfPeriod;
				_samplesToSilence -= samples;
				_amplitude = -_amplitude;
				remainingHalfPeriod = static_cast<size_t>(samplesPerHalfPeriod);
			}
			const auto samples = std::min(count, _samplesToSilence);
			for (auto i = samples; i > 0; --i)
				*base++ += _amplitude * (static_cast<float>(_samplesToSilence - i) / _samplingRate);
			_samplesToSilence -= samples;
			_halfPeriodRemaining = static_cast<double>(remainingHalfPeriod - count) / samplesPerHalfPeriod;
		}

		void start(double frequency, float amplitude) noexcept
		{
			if (!_samplesToSilence)
				_halfPeriodRemaining = 1.0;
			_frequency = frequency;
			_amplitude = std::copysign(std::clamp(amplitude, -1.f, 1.f), _amplitude);
			_samplesToSilence = frequency > 0.0 ? _maxSamples : 0;
		}

	private:
		const size_t _samplingRate;
		const size_t _maxSamples;
		double _frequency = 1.0;
		float _amplitude = 1.f;
		double _halfPeriodRemaining = 1.0;
		size_t _samplesToSilence = 0;
	};
}

namespace aulos
{
	class RendererImpl
	{
	public:
		RendererImpl(const CompositionImpl& composition, unsigned samplingRate)
			: _composition{ composition }, _samplingRate{ samplingRate } {}

	public:
		struct TrackState
		{
			Voice _voice;
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

	Renderer::Renderer(const Composition& composition, unsigned samplingRate)
		: _impl{ std::make_unique<RendererImpl>(static_cast<const CompositionImpl&>(composition), samplingRate) }
	{
		_impl->_tracks.reserve(_impl->_composition._tracks.size());
		for (const auto& track : _impl->_composition._tracks)
			_impl->_tracks.emplace_back(std::make_unique<RendererImpl::TrackState>(samplingRate, track.cbegin(), track.cend()));
	}

	Renderer::~Renderer() noexcept = default;

	size_t Renderer::render(void* buffer, size_t bufferBytes) noexcept
	{
		if (_impl->_tracks.empty())
			return 0;
		std::memset(buffer, 0, bufferBytes);
		size_t samplesRendered = 0;
		for (auto& track : _impl->_tracks)
		{
			size_t trackSamplesRendered = 0;
			while (track->_note != track->_end)
			{
				if (!track->_noteStarted)
				{
					track->_noteStarted = true;
					track->_noteSamplesRemaining = _impl->_stepSamples * track->_note->_duration;
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
}
