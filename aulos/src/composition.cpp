//
// This file is part of the Aulos toolkit.
// Copyright (C) 2019 Sergei Blagodarin.
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

#include "composition.hpp"

#include <algorithm>
#include <cassert>
#include <numeric>
#include <stdexcept>

namespace aulos
{
	CompositionImpl::CompositionImpl(const char* source)
	{
		size_t trackIndex = 0;

		const auto parseNote = [&](std::vector<NoteInfo>& track, const char* data, size_t baseOffset) {
			assert(baseOffset >= 0 && baseOffset < 12);
			if (*data == '#')
			{
				if (baseOffset == 11)
					throw std::runtime_error{ "Bad composition" };
				++baseOffset;
				++data;
			}
			else if (*data == 'b')
			{
				if (!baseOffset)
					throw std::runtime_error{ "Bad composition" };
				--baseOffset;
				++data;
			}
			if (*data < '0' || *data > '9')
				throw std::runtime_error{ "Bad composition" };
			track.emplace_back(static_cast<Note>((*data - '0') * 12 + baseOffset));
			return data + 1;
		};

		const auto parseNotes = [&](std::vector<NoteInfo>& track) {
			for (;;)
			{
				switch (*source++)
				{
				case '\0':
					return false;
				case '\r':
					if (*source == '\n')
						++source;
					[[fallthrough]];
				case '\n':
					return true;
				case '\t':
				case ' ':
					while (*source == ' ' || *source == '\t')
						++source;
					break;
				case '.':
					if (track.empty())
						track.emplace_back(Note::Silence);
					else
						++track.back()._duration;
					break;
				case 'A':
					source = parseNote(track, source, 9);
					break;
				case 'B':
					source = parseNote(track, source, 11);
					break;
				case 'C':
					source = parseNote(track, source, 0);
					break;
				case 'D':
					source = parseNote(track, source, 2);
					break;
				case 'E':
					source = parseNote(track, source, 4);
					break;
				case 'F':
					source = parseNote(track, source, 5);
					break;
				case 'G':
					source = parseNote(track, source, 7);
					break;
				default:
					throw std::runtime_error{ "Bad composition" };
				}
			}
		};

		std::vector<size_t> trackLengths;
		const auto alignTracks = [&] {
			if (_tracks.empty())
				return;
			trackLengths.resize(_tracks.size());
			for (size_t i = 0; i < _tracks.size(); ++i)
				trackLengths[i] = std::reduce(_tracks[i].cbegin(), _tracks[i].cend(), size_t{ 0 }, [](size_t size, const NoteInfo& noteInfo) { return size + noteInfo._duration; });
			const auto maxLength = *std::max_element(trackLengths.cbegin(), trackLengths.cend());
			for (size_t i = 0; i < _tracks.size(); ++i)
				if (!_tracks[i].empty())
					_tracks[i].back()._duration += maxLength - trackLengths[i];
				else
					_tracks[i].emplace_back(Note::Silence, maxLength);
		};

		for (;;)
		{
			switch (*source++)
			{
			case '\0':
				alignTracks();
				return;
			case '\r':
				if (*source == '\n')
					++source;
				[[fallthrough]];
			case '\n':
				trackIndex = 0;
				break;
			case '\t':
			case ' ':
				while (*source == ' ' || *source == '\t')
					++source;
				break;
			case ';':
				while (*source && *source != '\n' && *source != '\r')
					++source;
				continue;
			case '|':
				if (!trackIndex)
					alignTracks();
				if (trackIndex == _tracks.size())
					_tracks.emplace_back();
				if (!parseNotes(_tracks[trackIndex++]))
					return;
				break;
			default:
				throw std::runtime_error{ "Bad composition" };
			}
		}
	}

	std::unique_ptr<Composition> Composition::create(const char* source)
	{
		return std::make_unique<CompositionImpl>(source);
	}
}
