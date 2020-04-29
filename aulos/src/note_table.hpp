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
		static constexpr auto kNoteRatio = 1.0594630943592952645618252949463;

		constexpr NoteTable() noexcept
		{
			_frequencies[static_cast<size_t>(Note::A0)] = 27.5; // A0 for the standard A440 pitch (A4 = 440 Hz).
			for (auto a = static_cast<size_t>(Note::A1); a <= static_cast<size_t>(Note::A9); a += 12)
				_frequencies[a] = _frequencies[a - 12] * 2.0;
			for (size_t base = static_cast<size_t>(Note::C0); base < _frequencies.size() - 1; base += 12)
			{
				const auto a = base + static_cast<size_t>(Note::A0) - static_cast<size_t>(Note::C0);
				for (auto note = a; note > base; --note)
					_frequencies[note - 1] = _frequencies[note] / kNoteRatio;
				for (auto note = a; note < base + static_cast<size_t>(Note::B0) - static_cast<size_t>(Note::C0); ++note)
					_frequencies[note + 1] = _frequencies[note] * kNoteRatio;
			}
		}

		constexpr auto operator[](Note note) const noexcept
		{
			return static_cast<float>(_frequencies[static_cast<size_t>(note)]);
		}

	private:
		std::array<double, 120> _frequencies{};
	};

	inline const NoteTable kNoteTable;
}
