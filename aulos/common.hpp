// This file is part of the Aulos toolkit.
// Copyright (C) Sergei Blagodarin.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <chrono>
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
		Linear,          // Straight line (used for synthesizing square, rectangular, sawtooth and triangle waves).
		SmoothQuadratic, // Quadratic curve with zero derivative on the left.
		SharpQuadratic,  // Quadratic curve with zero derivative on the right.
		SmoothCubic,     // Cubic curve with parameterized derivative on the right.
		Quintic,         // Quintic curve with zero value and parameterized derivative in the middle.
		Cosine,          // Cosine curve.
	};

	// The left derivative of a smooth cubic shape is always zero (which means that one of the critial points
	// always coincides with the left end of the curve), and the right one is defined by the shape parameter:
	//  * [0, 1] - the derivative on the right starts at zero and increases until it becomes equal to the derivative of a linear shape;
	//  * [1, 2] - the second critical point moves right until it reaches positive infinity and the curve becomes quadratic;
	//  * [2, 3] - the second critical point moves from negative infinity to zero (i. e. to the left end of the curve).
	constexpr float kMinSmoothCubicShape = 0;
	constexpr float kMaxSmoothCubicShape = 3;

	constexpr float kMinQuinticShape = -1;
	constexpr float kMaxQuinticShape = 1;

	enum class EnvelopeShape
	{
		Linear,
		SmoothQuadratic2,
		SmoothQuadratic4,
		SharpQuadratic2,
		SharpQuadratic4,
	};

	struct EnvelopeChange
	{
		static constexpr auto kMaxDuration = std::chrono::milliseconds{ 60'000 };

		std::chrono::milliseconds _duration{ 0 };
		float _value = 0.f;
		EnvelopeShape _shape = EnvelopeShape::Linear;

		constexpr EnvelopeChange(const std::chrono::milliseconds& duration, float value, EnvelopeShape shape) noexcept
			: _duration{ duration }, _value{ value }, _shape{ shape } {}
	};

	// Specifies how a value changes over time.
	struct Envelope
	{
		std::vector<EnvelopeChange> _changes; // List of consecutive value changes.
	};

	enum class Polyphony
	{
		Chord, // Multiple notes which start simultaneously are rendered as a chord.
		Full,  // All distinct notes are rendered independently.
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
		float _stereoRadius = 0.f;
		float _stereoPan = 0.f;
		bool _stereoInversion = false;
		Polyphony _polyphony = Polyphony::Chord;
	};

	constexpr unsigned kMinSpeed = 1;  // Minimum composition playback speed (in steps per second).
	constexpr unsigned kMaxSpeed = 32; // Maximum composition playback speed (in steps per second).
}
