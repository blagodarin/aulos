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

#include "sequence_utils.hpp"

#include <aulos/data.hpp>

#include <array>

#include <QString>

namespace
{
	const std::array<char, 12> noteNames{ 'C', 'C', 'D', 'D', 'E', 'F', 'F', 'G', 'G', 'A', 'A', 'B' };
}

QString makeSequenceName(const aulos::SequenceData& sequence, bool rich)
{
	QString result;
	for (const auto& sound : sequence._sounds)
	{
		if (!rich && !result.isEmpty())
			result += ' ';
		for (size_t i = 1; i < sound._delay; ++i)
			result += rich ? "&nbsp;" : ". ";
		const auto octave = QString::number(static_cast<uint8_t>(sound._note) / 12);
		const auto note = static_cast<size_t>(sound._note) % 12;
		result += noteNames[note];
		const bool isSharpNote = note > 0 && noteNames[note - 1] == noteNames[note];
		if (!rich && isSharpNote)
			result += '#';
		result += rich ? QStringLiteral("<sub>%1</sub>").arg(octave) : octave;
		if (rich && isSharpNote)
			result += QStringLiteral("<sup>#</sup>");
	}
	return result;
}
