// This file is part of the Aulos toolkit.
// Copyright (C) Sergei Blagodarin.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <aulos/common.hpp>

#include <array>

namespace aulos
{
	class NoteFrequencies
	{
	public:
		constexpr auto operator[](Note note) const noexcept
		{
			return _values[static_cast<size_t>(note) % _values.size()] * static_cast<float>(1 << (static_cast<size_t>(note) / _values.size()));
		}

	private:
		// Lowest (subcontra) octave frequencies for twelve-tone equal temperament
		// and the standard musical pitch (A440) as defined in ISO 16.
		static constexpr std::array<float, kNotesPerOctave> _values{
			16.35159783128741466736562460f,
			17.32391443605450601554914585f,
			18.35404799483797251642393837f,
			19.44543648263005692102321996f,
			20.60172230705437060849011006f,
			21.82676446456274277783595254f,
			23.12465141947714993335595060f,
			24.49971474885933088035622065f,
			25.95654359874657115765261181f,
			27.5f,
			29.13523509488061977545019561f,
			30.86770632850775698942215888f,
		};
	};

	constexpr NoteFrequencies kNoteFrequencies;
}
