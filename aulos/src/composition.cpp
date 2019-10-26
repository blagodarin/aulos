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

#include <cassert>
#include <stdexcept>

namespace aulos
{
	CompositionImpl::CompositionImpl(const char* source)
	{
		bool startTrack = true;

		const auto parseNote = [&](const char* data, size_t baseOffset) {
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
			if (startTrack)
			{
				_tracks.emplace_back();
				startTrack = false;
			}
			_tracks.back().emplace_back(static_cast<Note>((*data - '0') * 12 + baseOffset));
			return data + 1;
		};

		for (;;)
		{
			switch (*source++)
			{
			case '\0':
				return;
			case '\n':
				startTrack = true;
				break;
			case '\r':
				if (*source == '\n')
					++source;
				startTrack = true;
				return;
			case '\t':
			case ' ':
				while (*source == '\t' || *source == ' ')
					++source;
				break;
			case '.':
				if (startTrack)
				{
					_tracks.emplace_back();
					startTrack = false;
				}
				if (_tracks.back().empty())
					_tracks.back().emplace_back(Note::Silence);
				else
					++_tracks.back().back()._duration;
				break;
			case 'A':
				source = parseNote(source, 9);
				break;
			case 'B':
				source = parseNote(source, 11);
				break;
			case 'C':
				source = parseNote(source, 0);
				break;
			case 'D':
				source = parseNote(source, 2);
				break;
			case 'E':
				source = parseNote(source, 4);
				break;
			case 'F':
				source = parseNote(source, 5);
				break;
			case 'G':
				source = parseNote(source, 7);
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
