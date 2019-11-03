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
#include <charconv>
#include <numeric>
#include <stdexcept>
#include <string>

namespace
{
	struct Location
	{
		size_t _line = 1;
		ptrdiff_t _offset = 1;
	};

	class CompositionError : public std::runtime_error
	{
	public:
		CompositionError(const Location& location, const std::string& message)
			: std::runtime_error("(" + std::to_string(location._line) + ':' + std::to_string(location._offset + 1) + ") " + message) {}
	};
}

namespace aulos
{
	CompositionImpl::CompositionImpl(const char* source)
	{
		size_t line = 1;
		const char* lineBase = source;
		size_t trackIndex = 0;

		const auto location = [&]() -> Location { return { line, source - lineBase }; };

		const auto skipSpaces = [&] {
			if (*source != ' ' && *source != '\t' && *source != '\n' && *source != '\r' && *source)
				throw CompositionError{ location(), "Space expected" };
			do
				++source;
			while (*source == ' ' || *source == '\t');
		};

		const auto readIdentifier = [&]() {
			if (*source < 'a' || *source > 'z')
				throw CompositionError{ location(), "Identifier expected" };
			const auto begin = source;
			do
				++source;
			while (*source >= 'a' && *source <= 'z');
			const std::string_view result{ begin, static_cast<size_t>(source - begin) };
			skipSpaces();
			return result;
		};

		const auto readFloat = [&]() -> float {
			if (*source < '0' || *source > '9')
				throw CompositionError{ location(), "Number expected" };
			const auto begin = source;
			do
				++source;
			while (*source >= '0' && *source <= '9');
			if (*source == '.')
				do
					++source;
				while (*source >= '0' && *source <= '9');
			float result;
			if (std::from_chars(begin, source, result).ec != std::errc{})
				throw CompositionError{ { line, begin - lineBase }, "Number expected" };
			skipSpaces();
			return result;
		};

		const auto parseNote = [&](std::vector<NoteInfo>& track, const char* data, size_t baseOffset) {
			assert(baseOffset >= 0 && baseOffset < 12);
			if (*data == '#')
			{
				if (baseOffset == 11)
					throw CompositionError{ location(), "Note overflow" };
				++baseOffset;
				++data;
			}
			else if (*data == 'b')
			{
				if (!baseOffset)
					throw CompositionError{ location(), "Note underflow" };
				--baseOffset;
				++data;
			}
			if (*data < '0' || *data > '9')
				throw CompositionError{ location(), "Bad note" };
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
					throw CompositionError{ location(), "Bad note" };
				}
			}
		};

		std::vector<size_t> trackLengths;
		size_t maxTrackLength = 0;
		const auto alignTracks = [&] {
			if (_tracks.empty())
				return;
			trackLengths.resize(_tracks.size());
			for (size_t i = 0; i < _tracks.size(); ++i)
				trackLengths[i] = std::reduce(_tracks[i].cbegin(), _tracks[i].cend(), size_t{ 0 }, [](size_t size, const NoteInfo& noteInfo) { return size + noteInfo._duration; });
			maxTrackLength = *std::max_element(trackLengths.cbegin(), trackLengths.cend());
			for (size_t i = 0; i < _tracks.size(); ++i)
				if (!_tracks[i].empty())
					_tracks[i].back()._duration += maxTrackLength - trackLengths[i];
				else
					_tracks[i].emplace_back(Note::Silence, maxTrackLength);
		};

		const auto parseCommand = [&](std::string_view command) {
			if (command == "envelope")
			{
				const auto type = readIdentifier();
				if (type == "adsr")
				{
					const auto attack = readFloat();
					const auto decay = readFloat();
					const auto sustain = readFloat();
					const auto release = readFloat();
					_envelope._parts.reserve(3);
					_envelope._parts.clear();
					_envelope._parts.emplace_back(0.f, 1.f, attack);
					_envelope._parts.emplace_back(1.f, sustain, decay);
					_envelope._parts.emplace_back(sustain, 0.f, release);
				}
				else if (type == "ahdsr")
				{
					const auto attack = readFloat();
					const auto hold = readFloat();
					const auto decay = readFloat();
					const auto sustain = readFloat();
					const auto release = readFloat();
					_envelope._parts.reserve(4);
					_envelope._parts.clear();
					_envelope._parts.emplace_back(0.f, 1.f, attack);
					_envelope._parts.emplace_back(1.f, 1.f, hold);
					_envelope._parts.emplace_back(1.f, sustain, decay);
					_envelope._parts.emplace_back(sustain, 0.f, release);
				}
				else
					throw CompositionError{ location(), "Bad envelope type" };
			}
			else if (command == "speed")
			{
				const auto speed = readFloat();
				if (speed < 1.f || speed > 32.f)
					throw CompositionError{ location(), "Bad speed" };
				_speed = speed;
			}
			else
				throw CompositionError{ location(), "Bad command" };
			if (*source && *source == '\n' && *source == '\r')
				throw CompositionError{ location(), "End of line expected" };
		};

		for (;;)
		{
			switch (*source)
			{
			case '\0':
				alignTracks();
				return;
			case '\r':
				if (source[1] == '\n')
					++source;
				[[fallthrough]];
			case '\n':
				++source;
				++line;
				lineBase = source;
				alignTracks();
				trackIndex = 0;
				break;
			case '\t':
			case ' ':
				do
					++source;
				while (*source == ' ' || *source == '\t');
				break;
			case ';':
				do
					++source;
				while (*source && *source != '\n' && *source != '\r');
				break;
			case '|':
				if (trackIndex == _tracks.size())
				{
					_tracks.emplace_back();
					if (maxTrackLength > 0)
						_tracks.back().emplace_back(Note::Silence, maxTrackLength);
				}
				++source;
				if (!parseNotes(_tracks[trackIndex++]))
					return;
				break;
			default:
				parseCommand(readIdentifier());
			}
		}
	}

	std::unique_ptr<Composition> Composition::create(const char* source)
	{
		return std::make_unique<CompositionImpl>(source);
	}
}
