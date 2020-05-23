// This file is part of the Aulos toolkit.
// Copyright (C) Sergei Blagodarin.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <aulos/voice.hpp>

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
		unsigned _weight = 0;
		std::vector<std::vector<Sound>> _sequences;
		std::vector<Fragment> _fragments;

		explicit Track(unsigned weight) noexcept
			: _weight{ weight } {}
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
		std::vector<Part> _parts;
		std::string _title;
		std::string _author;

		void load(const char* source);
	};
}
