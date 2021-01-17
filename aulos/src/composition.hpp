// This file is part of the Aulos toolkit.
// Copyright (C) Sergei Blagodarin.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <aulos/composition.hpp>

#include <aulos/common.hpp>

#include <primal/fixed.hpp>

#include <cstdint>
#include <string>

namespace aulos
{
	struct Fragment
	{
		size_t _delay = 0;
		size_t _sequence = 0;

		constexpr Fragment(size_t delay, size_t sequence) noexcept
			: _delay{ delay }, _sequence{ sequence } {}
	};

	struct Track
	{
		TrackProperties _properties;
		std::vector<std::vector<Sound>> _sequences;
		std::vector<Fragment> _fragments;
	};

	struct Part
	{
		VoiceData _voice;
		std::string _voiceName;
		std::vector<Track> _tracks;
	};

	struct CompositionImpl final : public Composition
	{
		unsigned _speed = kMinSpeed;
		unsigned _loopOffset = 0;
		unsigned _loopLength = 0;
		primal::Fixed<uint16_t, 4> _gainDivisor{ 1.f };
		std::vector<Part> _parts;
		std::string _title;
		std::string _author;

		void load(const char* source);
	};
}
