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

	struct SampledEnvelope
	{
	public:
		struct Point
		{
			size_t _delay;
			float _value;
		};

		SampledEnvelope(const aulos::Envelope& envelope, size_t samplingRate) noexcept
		{
			assert(envelope._points.size() <= _points.size());
			for (const auto& point : envelope._points)
			{
				assert(point._delay >= 0.f);
				_points[_size++] = { static_cast<size_t>(double{ point._delay } * samplingRate), point._value };
			}
		}

		const Point* begin() const noexcept { return _points.data(); }
		const Point* end() const noexcept { return _points.data() + _size; }

	private:
		size_t _size = 0;
		std::array<Point, 4> _points{};
	};

	class AmplitudeModulator
	{
	public:
		AmplitudeModulator(const SampledEnvelope& envelope) noexcept
			: _envelope{ envelope } {}

		void start(float value) noexcept
		{
			_baseValue = value;
			_nextPoint = std::find_if(_envelope.begin(), _envelope.end(), [](const SampledEnvelope::Point& point) { return point._delay > 0; });
			_offset = 0;
			_currentValue = _baseValue;
		}

		float advance() noexcept
		{
			assert(_nextPoint != _envelope.end());
			assert(_nextPoint->_delay > 0);
			assert(_offset < _nextPoint->_delay);
			const auto progress = static_cast<float>(_offset) / _nextPoint->_delay;
			_currentValue = _baseValue * (1.f - progress) + _nextPoint->_value * progress;
			++_offset;
			return _currentValue;
		}

		size_t partSamplesRemaining() const noexcept
		{
			assert(_nextPoint != _envelope.end());
			return _nextPoint->_delay - _offset;
		}

		void stop() noexcept
		{
			_nextPoint = _envelope.end();
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

		constexpr float value() const noexcept
		{
			return _currentValue;
		}

	private:
		const SampledEnvelope& _envelope;
		float _baseValue = 0.f;
		const SampledEnvelope::Point* _nextPoint = _envelope.end();
		size_t _offset = 0;
		float _currentValue = 0.f;
	};

	class FrequencyModulator
	{
	public:
		FrequencyModulator(const SampledEnvelope& envelope) noexcept
			: _envelope{ envelope } {}

		void start(double value) noexcept
		{
			_baseValue = value;
			_nextPoint = std::find_if(_envelope.begin(), _envelope.end(), [](const SampledEnvelope::Point& point) { return point._delay > 0; });
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

	class Voice
	{
	public:
		virtual ~Voice() noexcept = default;

		virtual void start(double frequency, float amplitude) noexcept = 0;
		virtual void generate(float* buffer, size_t maxSamples) noexcept = 0;
	};

	class TwoPartWaveBase : public Voice
	{
	public:
		TwoPartWaveBase(double asymmetry, double oscillation, const SampledEnvelope& amplitude, const SampledEnvelope& frequency, size_t samplingRate) noexcept
			: _asymmetry{ (1.0 + std::clamp(asymmetry, -1.0, 1.0)) / 2.0 }
			, _oscillation{ oscillation }
			, _amplitudeModulator{ amplitude }
			, _frequencyModulator{ frequency }
			, _samplingRate{ static_cast<double>(samplingRate) }
		{
		}

		void start(double frequency, float amplitude) noexcept override
		{
			if (frequency <= 0.0)
			{
				_amplitudeModulator.stop();
				return;
			}
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
				partRemaining = _partSamplesRemaining / _partSamples[_partIndex];
			}
			_frequencyModulator.start(1.0);
			_frequency = frequency;
			updatePeriodParts();
			_partSamplesRemaining = _partSamples[_partIndex] * partRemaining;
			advance(0);
		}

	protected:
		void advance(size_t samples) noexcept
		{
			_frequencyModulator.advance(samples);
			auto remaining = _partSamplesRemaining - samples;
			while (remaining <= 0.0)
			{
				_amplitude = -_amplitude;
				_partIndex = 1 - _partIndex;
				if (!_partIndex)
					updatePeriodParts();
				remaining += _partSamples[_partIndex];
			}
			_partSamplesRemaining = remaining;
		}

		void updatePeriodParts() noexcept
		{
			const auto periodSamples = _samplingRate / (_frequency * _frequencyModulator.value());
			_partSamples[0] = periodSamples * _asymmetry;
			_partSamples[1] = periodSamples - _partSamples[0];
		}

	protected:
		const double _asymmetry;
		const double _oscillation;
		AmplitudeModulator _amplitudeModulator;
		FrequencyModulator _frequencyModulator;
		const double _samplingRate;
		float _amplitude = 0.f;
		double _frequency = 0.0;
		size_t _partIndex = 0;
		std::array<double, 2> _partSamples{};
		double _partSamplesRemaining = 0.0;
	};

	template <typename Oscillator>
	class TwoPartWave final : public TwoPartWaveBase
	{
	public:
		using TwoPartWaveBase::TwoPartWaveBase;

		void generate(float* buffer, size_t maxSamples) noexcept override
		{
			assert(maxSamples > 0);
			while (_amplitudeModulator.update())
			{
				const auto samplesToGenerate = std::min(static_cast<size_t>(std::ceil(_partSamplesRemaining)), std::min(maxSamples, _amplitudeModulator.partSamplesRemaining()));
				Oscillator oscillator{ _partSamplesRemaining, _partSamples[_partIndex], _oscillation };
				for (size_t i = 0; i < samplesToGenerate; ++i)
					buffer[i] += _amplitude * oscillator() * _amplitudeModulator.advance();
				advance(samplesToGenerate);
				buffer += samplesToGenerate;
				maxSamples -= samplesToGenerate;
				if (!maxSamples)
					break;
			}
		}
	};

	struct LinearOscillator
	{
		const double _increment;
		double _value;

		constexpr LinearOscillator(double remainingSamples, double totalSamples, double oscillation) noexcept
			: _increment{ 2.0 * oscillation / totalSamples }, _value{ 1.0 - _increment * (remainingSamples - 1.0) } {}

		constexpr float operator()() noexcept
		{
			return static_cast<float>(_value += _increment);
		}
	};

	std::unique_ptr<Voice> createVoice(const aulos::Wave& wave, const SampledEnvelope& amplitude, const SampledEnvelope& frequency, unsigned samplingRate)
	{
		switch (wave._type)
		{
		case aulos::WaveType::Linear: return std::make_unique<TwoPartWave<LinearOscillator>>(wave._asymmetry, wave._oscillation, amplitude, frequency, samplingRate);
		}
		return {};
	}
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
			const auto totalWeight = static_cast<float>(std::reduce(_composition._tracks.cbegin(), _composition._tracks.cend(), 0u, [](unsigned weight, const Track& track) { return weight + track._weight; }));
			_tracks.reserve(_composition._tracks.size());
			for (const auto& track : _composition._tracks)
				_tracks.emplace_back(std::make_unique<RendererImpl::TrackState>(track._wave, track._amplitude, track._frequency, track._weight / totalWeight, track._notes, samplingRate));
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
						track->_voice->start(kNoteTable[track->_note->_note], track->_normalizedWeight);
					}
					const auto samplesToGenerate = bufferBytes / kSampleSize - trackSamplesRendered;
					if (!samplesToGenerate)
						break;
					const auto samplesGenerated = std::min(track->_noteSamplesRemaining, samplesToGenerate);
					track->_voice->generate(static_cast<float*>(buffer) + trackSamplesRendered, samplesGenerated);
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
			SampledEnvelope _amplitude;
			SampledEnvelope _frequency;
			std::unique_ptr<Voice> _voice;
			const float _normalizedWeight;
			std::vector<NoteInfo>::const_iterator _note;
			const std::vector<NoteInfo>::const_iterator _end;
			bool _noteStarted = false;
			size_t _noteSamplesRemaining = 0;

			TrackState(const Wave& wave, const Envelope& amplitude, const Envelope& frequency, float normalizedWeight, const std::vector<NoteInfo>& notes, unsigned samplingRate)
				: _amplitude{ amplitude, samplingRate }
				, _frequency{ frequency, samplingRate }
				, _voice{ createVoice(wave, _amplitude, _frequency, samplingRate) }
				, _normalizedWeight{ normalizedWeight }
				, _note{ notes.cbegin() }
				, _end{ notes.cend() }
			{
			}
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
