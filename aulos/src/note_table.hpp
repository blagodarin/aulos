// This file is part of the Aulos toolkit.
// Copyright (C) Sergei Blagodarin.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <aulos/common.hpp>

#include <array>

namespace aulos
{
	class NoteTable
	{
	public:
		static constexpr size_t kNoteCount = 120;
		static constexpr double kNoteRatio = 1.0594630943592952645618252949463;
		static constexpr size_t kOctaveLength = 12;

		constexpr NoteTable() noexcept
		{
			_frequencies[static_cast<size_t>(Note::A4)] = 440; // Standard musical pitch (A440) as defined in ISO 16.
			for (auto i = static_cast<size_t>(Note::A4); i > static_cast<size_t>(Note::A0); i -= kOctaveLength)
				_frequencies[i - kOctaveLength] = _frequencies[i] / 2;
			for (auto i = static_cast<size_t>(Note::A4); i < static_cast<size_t>(Note::A9); i += kOctaveLength)
				_frequencies[i + kOctaveLength] = _frequencies[i] * 2;
			for (size_t c = static_cast<size_t>(Note::C0); c <= static_cast<size_t>(Note::C9); c += kOctaveLength)
			{
				const auto a = c + static_cast<size_t>(Note::A0) - static_cast<size_t>(Note::C0);
				double frequency = _frequencies[a];
				for (auto note = a; note > c; --note)
				{
					frequency /= kNoteRatio;
					_frequencies[note - 1] = static_cast<float>(frequency);
				}
				frequency = _frequencies[a];
				for (auto note = a + 1; note < c + kOctaveLength; ++note)
				{
					frequency *= kNoteRatio;
					_frequencies[note] = static_cast<float>(frequency);
				}
			}
		}

		constexpr auto operator[](Note note) const noexcept
		{
			return _frequencies[static_cast<size_t>(note)];
		}

		static constexpr int stereoDelay(Note note, int offset, int radius) noexcept
		{
			constexpr auto kLastNoteIndex = static_cast<int>(kNoteCount - 1);
			return offset + radius * (2 * static_cast<int>(note) - kLastNoteIndex) / kLastNoteIndex;
		}

	private:
		std::array<float, kNoteCount> _frequencies{};
	};

	constexpr NoteTable kNoteTable;
}
