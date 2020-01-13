//
// This file is part of the Aulos toolkit.
// Copyright (C) 2020 Sergei Blagodarin.
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

#pragma once

#include <aulos/playback.hpp>

#include <string>
#include <vector>

namespace aulos
{
	enum class Note : uint8_t
	{
		// clang-format off
		C0, Db0, D0, Eb0, E0, F0, Gb0, G0, Ab0, A0, Bb0, B0,
		C1, Db1, D1, Eb1, E1, F1, Gb1, G1, Ab1, A1, Bb1, B1,
		C2, Db2, D2, Eb2, E2, F2, Gb2, G2, Ab2, A2, Bb2, B2,
		C3, Db3, D3, Eb3, E3, F3, Gb3, G3, Ab3, A3, Bb3, B3,
		C4, Db4, D4, Eb4, E4, F4, Gb4, G4, Ab4, A4, Bb4, B4,
		C5, Db5, D5, Eb5, E5, F5, Gb5, G5, Ab5, A5, Bb5, B5,
		C6, Db6, D6, Eb6, E6, F6, Gb6, G6, Ab6, A6, Bb6, B6,
		C7, Db7, D7, Eb7, E7, F7, Gb7, G7, Ab7, A7, Bb7, B7,
		C8, Db8, D8, Eb8, E8, F8, Gb8, G8, Ab8, A8, Bb8, B8,
		C9, Db9, D9, Eb9, E9, F9, Gb9, G9, Ab9, A9, Bb9, B9,
		// clang-format on
	};

	struct Sound
	{
		Note _note = Note::C0;
		size_t _pause = 0; // Pause before the next sound in a sequence.

		constexpr Sound(Note note, size_t pause) noexcept
			: _note{ note }, _pause{ pause } {}
	};

	// Wave types.
	enum class Wave
	{
		Linear, // Linear wave (includes square, rectangular, sawtooth and triangle waveforms).
	};

	struct Point
	{
		float _delay = 0.f;
		float _value = 0.f;

		constexpr Point(float delay, float value) noexcept
			: _delay{ delay }, _value{ value } {}
	};

	constexpr auto kMaxEnvelopePartDuration = 60.f; // Maximum delay between consecutive envelope points (in seconds).

	// Specifies how a value changes over time.
	struct Envelope
	{
		float _initial = 0.f;        // Initial value.
		std::vector<Point> _changes; // List of consecutive value changes.

		Envelope(float initial) noexcept
			: _initial{ initial } {}
	};

	// Specifies how to generate a waveform for a sound.
	struct Voice
	{
		Wave _wave = Wave::Linear;
		float _oscillation = 1.f;
		Envelope _amplitudeEnvelope{ 0.f };
		Envelope _frequencyEnvelope{ 1.f };
		Envelope _asymmetryEnvelope{ 0.f };
		std::string _name;
	};

	struct SequenceData
	{
		std::vector<Sound> _sounds;
	};

	// Specifies when to play a sequence.
	struct FragmentData
	{
		size_t _delay = 0; // Steps from the beginning of the previous fragment.
		std::shared_ptr<const SequenceData> _sequence;

		FragmentData(size_t delay, const std::shared_ptr<const SequenceData>& sequence) noexcept
			: _delay{ delay }, _sequence{ sequence } {}
	};

	// Specifies playback actions for a single voice instance.
	struct TrackData
	{
		size_t _voice = 0;
		unsigned _weight = 0;
		std::vector<std::shared_ptr<SequenceData>> _sequences;
		std::vector<FragmentData> _fragments;

		TrackData(size_t voice, unsigned weight) noexcept
			: _voice{ voice }, _weight{ weight } {}
	};

	constexpr float kMinSpeed = 1.f;  // Minimum composition playback speed (in steps per second).
	constexpr float kMaxSpeed = 32.f; // Maximum composition playback speed (in steps per second).

	// Contains composition data in an editable format.
	struct CompositionData
	{
		float _speed = kMinSpeed;
		std::vector<Voice> _voices;
		std::vector<TrackData> _tracks;

		CompositionData() = default;
		CompositionData(const Composition&);

		[[nodiscard]] std::unique_ptr<Composition> pack() const;
	};

	// Generates PCM audio for a voice.
	class VoiceRenderer : public Renderer
	{
	public:
		[[nodiscard]] static std::unique_ptr<VoiceRenderer> create(const Voice&, unsigned samplingRate);

		virtual ~VoiceRenderer() noexcept = default;

		[[nodiscard]] virtual size_t duration() const noexcept = 0;
		virtual void start(Note, float amplitude) noexcept = 0;
	};
}
