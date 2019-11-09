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
		struct Part
		{
			float _left;
			float _right;
			size_t _samples;
		};

		SampledEnvelope(const aulos::Envelope& envelope, size_t samplingRate) noexcept
		{
			float last = 0.f;
			for (const auto& point : envelope._points)
			{
				assert(point._delay >= 0.f);
				if (const auto samples = static_cast<size_t>(double{ point._delay } * samplingRate); samples > 0)
				{
					assert(_size < _parts.size());
					_parts[_size++] = { last, point._value, samples };
				}
				last = point._value;
			}
		}

		const Part* begin() const noexcept { return _parts.data(); }
		const Part* end() const noexcept { return _parts.data() + _size; }

	private:
		size_t _size = 0;
		std::array<Part, 4> _parts{};
	};

	class AmplitudeState
	{
	public:
		AmplitudeState(const SampledEnvelope& envelope) noexcept
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

		void stop() noexcept
		{
			_current = _envelope.end();
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
		const SampledEnvelope& _envelope;
		const SampledEnvelope::Part* _current = _envelope.end();
		float _currentLeft = 0.f;
		float _lastValue = 0.f;
		size_t _currentOffset = 0;
	};

	class FrequencyState
	{
	public:
		FrequencyState(const SampledEnvelope& envelope, size_t samplingRate) noexcept
			: _envelope{ envelope }, _samplingRate{ static_cast<double>(samplingRate) } {}

		double currentPeriodSamples() const noexcept { return _samplingRate / _currentFrequency; }

		void start(double frequency) noexcept
		{
			_baseFrequency = frequency;
			_offset = 0;
			_current = _envelope.begin();
			_currentFrequency = _current != _envelope.end() ? _current->_left * _baseFrequency : _baseFrequency;
		}

		void advance(size_t samples) noexcept
		{
			for (;;)
			{
				if (_current == _envelope.end())
					return;
				const auto remainingSamples = _current->_samples - _offset;
				if (remainingSamples <= samples)
				{
					samples -= remainingSamples;
					_offset = 0;
					_currentFrequency = _current->_right * _baseFrequency;
					++_current;
					continue;
				}
				_offset += samples;
				const auto progress = static_cast<double>(_offset) / _current->_samples;
				_currentFrequency = (_current->_left * (1.0 - progress) + _current->_right * progress) * _baseFrequency;
				break;
			}
		}

	private:
		const SampledEnvelope& _envelope;
		const double _samplingRate;
		double _baseFrequency = 0.0;
		const SampledEnvelope::Part* _current = _envelope.end();
		size_t _offset = 0;
		double _currentFrequency = 0.0;
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
		TwoPartWaveBase(double parameter, const SampledEnvelope& amplitude, const SampledEnvelope& frequency, size_t samplingRate) noexcept
			: _parameter{ (1.0 + std::clamp(parameter, -1.0, 1.0)) / 2.0 }, _amplitudeState{ amplitude }, _frequencyState{ frequency, samplingRate } {}

		void start(double frequency, float amplitude) noexcept override
		{
			if (frequency <= 0.0)
			{
				_amplitudeState.stop();
				return;
			}
			const auto clampedAmplitude = std::clamp(amplitude, -1.f, 1.f);
			double partRemaining;
			if (_amplitudeState.start())
			{
				_baseAmplitude = clampedAmplitude;
				_partIndex = 0;
				partRemaining = 1.0;
			}
			else
			{
				_baseAmplitude = std::copysign(clampedAmplitude, _baseAmplitude);
				partRemaining = _partSamplesRemaining / _partSamples[_partIndex];
			}
			_frequencyState.start(frequency);
			const auto periodSamples = _frequencyState.currentPeriodSamples();
			_partSamples[0] = periodSamples * _parameter;
			_partSamples[1] = periodSamples - _partSamples[0];
			_partSamplesRemaining = _partSamples[_partIndex] * partRemaining;
			advance(0);
		}

	protected:
		void advance(size_t samples) noexcept
		{
			_frequencyState.advance(samples);
			auto remaining = _partSamplesRemaining - samples;
			while (remaining <= 0.0)
			{
				_baseAmplitude = -_baseAmplitude;
				_partIndex = 1 - _partIndex;
				if (!_partIndex)
				{
					const auto periodSamples = _frequencyState.currentPeriodSamples();
					_partSamples[0] = periodSamples * _parameter;
					_partSamples[1] = periodSamples - _partSamples[0];
				}
				remaining += _partSamples[_partIndex];
			}
			_partSamplesRemaining = remaining;
		}

	protected:
		const double _parameter;
		AmplitudeState _amplitudeState;
		FrequencyState _frequencyState;
		float _baseAmplitude = 0.f;
		size_t _partIndex = 0;
		std::array<double, 2> _partSamples{};
		double _partSamplesRemaining = 0.0;
	};

	template <typename Oscillator>
	class TwoPartWave final : public TwoPartWaveBase
	{
	public:
		TwoPartWave(double parameter, const SampledEnvelope& amplitude, const SampledEnvelope& frequency, size_t samplingRate) noexcept
			: TwoPartWaveBase{ parameter, amplitude, frequency, samplingRate } {}

		void generate(float* buffer, size_t maxSamples) noexcept override
		{
			assert(maxSamples > 0);
			while (_amplitudeState.update())
			{
				const auto samplesToGenerate = std::min(static_cast<size_t>(std::ceil(_partSamplesRemaining)), std::min(maxSamples, _amplitudeState.partSamplesRemaining()));
				Oscillator oscillator{ _partSamplesRemaining, _partSamples[_partIndex] };
				for (size_t i = 0; i < samplesToGenerate; ++i)
					buffer[i] += _baseAmplitude * oscillator() * _amplitudeState.advance();
				advance(samplesToGenerate);
				buffer += samplesToGenerate;
				maxSamples -= samplesToGenerate;
				if (!maxSamples)
					break;
			}
		}
	};

	struct RectangleOscillator
	{
		constexpr RectangleOscillator(double, double) noexcept {}

		constexpr float operator()() const noexcept
		{
			return 1.f;
		}
	};

	struct TriangleOscillator
	{
		double _value;
		const double _increment;

		constexpr TriangleOscillator(double remainingSamples, double totalSamples) noexcept
			: _value{ 1.0 - 2.0 * (remainingSamples - 1.0) / totalSamples }, _increment{ 2.0 / totalSamples } {}

		constexpr float operator()() noexcept
		{
			return static_cast<float>(_value += _increment);
		}
	};

	std::unique_ptr<Voice> createVoice(const aulos::Wave& wave, const SampledEnvelope& amplitude, const SampledEnvelope& frequency, unsigned samplingRate)
	{
		switch (wave._type)
		{
		case aulos::WaveType::Rectangle: return std::make_unique<TwoPartWave<RectangleOscillator>>(wave._parameter, amplitude, frequency, samplingRate);
		case aulos::WaveType::Triangle: return std::make_unique<TwoPartWave<TriangleOscillator>>(wave._parameter, amplitude, frequency, samplingRate);
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
				: _amplitude{ amplitude, samplingRate }, _frequency{ frequency, samplingRate }, _voice{ createVoice(wave, _amplitude, _frequency, samplingRate) }, _normalizedWeight{ normalizedWeight }, _note{ notes.cbegin() }, _end{ notes.cend() } {}
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
