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

#include <map>
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
		size_t _delay = 0; // Offset from the previous sound in a sequence.
		Note _note = Note::C0;

		constexpr Sound(size_t delay, Note note) noexcept
			: _delay{ delay }, _note{ note } {}
	};

	// Shape types.
	enum class WaveShape
	{
		Linear,     // Straight line (used for synthesizing square, rectangular, sawtooth and triangle waves).
		Quadratic1, // Quadratic curve with zero derivative on the left.
		Quadratic2, // Quadratic curve with zero derivative on the right.
		Cubic,      // Cubic curve.
		Quintic,    // Quintic curve with zero value in the middle.
		Cosine,     // Cosine curve.
	};

	struct Point
	{
		static constexpr auto kMaxDelayMs = 60'000u; // Maximum delay between consecutive envelope points.

		unsigned _delayMs = 0;
		float _value = 0.f;

		constexpr Point(unsigned delay, float value) noexcept
			: _delayMs{ delay }, _value{ value } {}
	};

	// Specifies how a value changes over time.
	struct Envelope
	{
		std::vector<Point> _points; // List of consecutive value changes.
	};

	// Specifies how to generate a waveform for a sound.
	struct VoiceData
	{
		WaveShape _waveShape = WaveShape::Linear;
		Envelope _amplitudeEnvelope;
		Envelope _frequencyEnvelope;
		Envelope _asymmetryEnvelope;
		Envelope _oscillationEnvelope;
		float _waveShapeParameter = 0.f;
		float _stereoDelay = 0.f;
		float _stereoPan = 0.f;
		bool _stereoInversion = false;
	};

	struct SequenceData
	{
		std::vector<Sound> _sounds;
	};

	struct TrackData
	{
		unsigned _weight = 0;
		std::vector<std::shared_ptr<SequenceData>> _sequences;
		std::map<size_t, std::shared_ptr<SequenceData>> _fragments;

		TrackData(unsigned weight) noexcept
			: _weight{ weight } {}
	};

	struct PartData
	{
		std::shared_ptr<VoiceData> _voice;
		std::string _voiceName;
		std::vector<std::shared_ptr<TrackData>> _tracks;

		PartData(const std::shared_ptr<VoiceData>& voice) noexcept
			: _voice{ voice } {}
	};

	constexpr unsigned kMinSpeed = 1;  // Minimum composition playback speed (in steps per second).
	constexpr unsigned kMaxSpeed = 32; // Maximum composition playback speed (in steps per second).

	// Contains composition data in an editable format.
	struct CompositionData
	{
		unsigned _speed = kMinSpeed;
		std::vector<std::shared_ptr<PartData>> _parts;
		std::string _title;
		std::string _author;

		CompositionData() = default;
		CompositionData(const Composition&);

		[[nodiscard]] std::unique_ptr<Composition> pack() const;
	};

	// Generates PCM audio for a voice.
	class VoiceRenderer : public Renderer
	{
	public:
		[[nodiscard]] static std::unique_ptr<VoiceRenderer> create(const VoiceData&, unsigned samplingRate, unsigned channels);

		virtual ~VoiceRenderer() noexcept = default;

		virtual void start(Note, float amplitude) noexcept = 0;
	};
}
