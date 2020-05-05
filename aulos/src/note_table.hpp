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

#include <aulos/data.hpp>

#include <array>

namespace aulos
{
	class NoteTable
	{
	public:
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

	private:
		std::array<float, 120> _frequencies{};
	};

	constexpr NoteTable kNoteTable;
}
