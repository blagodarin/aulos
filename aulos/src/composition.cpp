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
#include <optional>
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
	void CompositionImpl::load(const char* source)
	{
		enum class Section
		{
			Initial,
			Voice,
			Sequences,
			Fragments,
		};

		size_t line = 1;
		const char* lineBase = source;
		Section currentSection = Section::Initial;
		VoiceData* currentVoice = nullptr;
		unsigned lastFragmentDelay = 0;

		const auto location = [&]() -> Location { return { line, source - lineBase }; };

		const auto skipSpaces = [&] {
			if (*source != ' ' && *source != '\t' && *source != '\n' && *source != '\r' && *source)
				throw CompositionError{ location(), "Space expected" };
			while (*source == ' ' || *source == '\t')
				++source;
		};

		const auto consumeEndOfLine = [&] {
			if (*source == '\r')
			{
				++source;
				if (*source == '\n')
					++source;
			}
			else if (*source == '\n')
				++source;
			else
			{
				if (*source)
					throw CompositionError{ location(), "End of line expected" };
				return;
			}
			++line;
			lineBase = source;
		};

		const auto readIdentifier = [&] {
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

		const auto tryReadUnsigned = [&]() -> std::optional<unsigned> {
			if (*source < '0' || *source > '9')
				return {};
			const auto begin = source;
			do
				++source;
			while (*source >= '0' && *source <= '9');
			unsigned result;
			if (std::from_chars(begin, source, result).ec != std::errc{})
				throw CompositionError{ { line, begin - lineBase }, "Number expected" };
			skipSpaces();
			return result;
		};

		const auto readUnsigned = [&] {
			const auto result = tryReadUnsigned();
			if (!result)
				throw CompositionError{ location(), "Number expected" };
			return *result;
		};

		const auto tryReadFloat = [&](float min, float max) -> std::optional<float> {
			if (*source < '0' || *source > '9')
				return {};
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
				throw CompositionError{ { line, begin - lineBase }, "Bad number" };
			if (result < min || result > max)
				throw CompositionError{ { line, begin - lineBase }, "Number is out of range" };
			skipSpaces();
			return result;
		};

		const auto readFloat = [&](float min, float max) {
			const auto result = tryReadFloat(min, max);
			if (!result)
				throw CompositionError{ location(), "Number expected" };
			return *result;
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

		const auto parseCommand = [&](std::string_view command) {
			if (command == "amplitude")
			{
				if (currentSection != Section::Voice)
					throw CompositionError{ location(), "Unexpected command" };
				constexpr auto maxPartDuration = 60.f;
				auto& envelope = currentVoice->_amplitude._points;
				if (const auto type = readIdentifier(); type == "adsr")
				{
					const auto attack = readFloat(0.f, maxPartDuration);
					const auto decay = readFloat(0.f, maxPartDuration);
					const auto sustain = readFloat(0.f, 1.f);
					const auto release = readFloat(0.f, maxPartDuration);
					envelope.reserve(3);
					envelope.clear();
					envelope.emplace_back(attack, 1.f);
					envelope.emplace_back(decay, sustain);
					envelope.emplace_back(release, 0.f);
				}
				else if (type == "ahdsr")
				{
					const auto attack = readFloat(0.f, maxPartDuration);
					const auto hold = readFloat(0.f, maxPartDuration);
					const auto decay = readFloat(0.f, maxPartDuration);
					const auto sustain = readFloat(0.f, 1.f);
					const auto release = readFloat(0.f, maxPartDuration);
					envelope.reserve(4);
					envelope.clear();
					envelope.emplace_back(attack, 1.f);
					envelope.emplace_back(hold, 1.f);
					envelope.emplace_back(decay, sustain);
					envelope.emplace_back(release, 0.f);
				}
				else
					throw CompositionError{ location(), "Bad envelope type" };
			}
			else if (command == "asymmetry")
			{
				if (currentSection != Section::Voice)
					throw CompositionError{ location(), "Unexpected command" };
				std::vector<Envelope::Point> points;
				points.emplace_back(0.f, readFloat(0.f, 1.f));
				while (const auto delay = tryReadFloat(0.f, 60.f))
					points.emplace_back(*delay, readFloat(0.f, 1.f));
				currentVoice->_asymmetry._points = std::move(points);
			}
			else if (command == "frequency")
			{
				if (currentSection != Section::Voice)
					throw CompositionError{ location(), "Unexpected command" };
				constexpr auto minFrequency = std::numeric_limits<float>::min();
				std::vector<Envelope::Point> points;
				points.emplace_back(0.f, readFloat(minFrequency, 1.f));
				while (const auto delay = tryReadFloat(0.f, 60.f))
					points.emplace_back(*delay, readFloat(minFrequency, 1.f));
				currentVoice->_frequency._points = std::move(points);
			}
			else if (command == "wave")
			{
				if (currentSection != Section::Voice)
					throw CompositionError{ location(), "Unexpected command" };
				if (const auto type = readIdentifier(); type == "linear")
				{
					const auto oscillation = tryReadFloat(0.f, 1.f);
					currentVoice->_wave._type = WaveType::Linear;
					currentVoice->_wave._oscillation = oscillation ? *oscillation : 0.f;
				}
				else if (type == "rectangle")
				{
					currentVoice->_wave._type = WaveType::Linear;
					currentVoice->_wave._oscillation = 0.0;
				}
				else if (type == "triangle")
				{
					currentVoice->_wave._type = WaveType::Linear;
					currentVoice->_wave._oscillation = 1.0;
				}
				else
					throw CompositionError{ location(), "Bad voice wave type" };
			}
			else if (command == "weight")
			{
				if (currentSection != Section::Voice)
					throw CompositionError{ location(), "Unexpected command" };
				const auto weight = readUnsigned();
				if (!weight || weight > 256)
					throw CompositionError{ location(), "Bad voice weight" };
				consumeEndOfLine();
				currentVoice->_weight = weight;
			}
			else if (command == "speed")
			{
				if (currentSection != Section::Initial)
					throw CompositionError{ location(), "Unexpected command" };
				setSpeed(readFloat(kMinSpeed, kMaxSpeed));
				consumeEndOfLine();
			}
			else
				throw CompositionError{ location(), "Unknown command \"" + std::string{ command } + "\"" };
			if (*source && *source == '\n' && *source == '\r')
				throw CompositionError{ location(), "End of line expected" };
		};

		for (;;)
		{
			switch (*source)
			{
			case '\0':
				return;
			case '\r':
			case '\n':
				consumeEndOfLine();
				break;
			case '\t':
			case ' ':
				do
					++source;
				while (*source == ' ' || *source == '\t');
				break;
			case '0':
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
			case '8':
			case '9':
				switch (currentSection)
				{
				case Section::Sequences:
					if (readUnsigned() != _sequences.size() + 1)
						throw CompositionError{ location(), "Bad sequence index" };
					parseNotes(_sequences.emplace_back());
					break;
				case Section::Fragments:
				{
					const auto nextFragmentDelay = readUnsigned();
					for (auto trackIndex = readUnsigned();;)
					{
						if (!trackIndex || trackIndex > _voices.size())
							throw CompositionError{ location(), "Bad fragment track index" };
						const auto sequenceIndex = readUnsigned();
						if (!sequenceIndex || sequenceIndex > _sequences.size())
							throw CompositionError{ location(), "Bad fragment sequence index" };
						_fragments.emplace_back(std::exchange(lastFragmentDelay, 0), trackIndex - 1, sequenceIndex - 1);
						if (const auto nextIndex = tryReadUnsigned())
							trackIndex = *nextIndex;
						else
							break;
					}
					lastFragmentDelay = nextFragmentDelay;
					break;
				}
				default:
					throw CompositionError{ location(), "Unexpected token" };
				}
				break;
			case ';':
				do
					++source;
				while (*source && *source != '\n' && *source != '\r');
				break;
			case '@':
				++source;
				if (const auto section = readIdentifier(); section == "voice")
				{
					const auto voiceIndex = readUnsigned();
					if (voiceIndex != _voices.size() + 1)
						throw CompositionError{ location(), "Bad voice index" };
					_voices.emplace_back();
					consumeEndOfLine();
					currentSection = Section::Voice;
					currentVoice = &_voices.back();
				}
				else if (section == "sequences")
				{
					consumeEndOfLine();
					currentSection = Section::Sequences;
				}
				else if (section == "fragments")
				{
					consumeEndOfLine();
					currentSection = Section::Fragments;
				}
				else
					throw CompositionError{ location(), "Unknown section \"@" + std::string{ section } + "\"" };
				break;
			default:
				parseCommand(readIdentifier());
			}
		}
	}

	bool CompositionImpl::setSpeed(float speed)
	{
		if (speed < kMinSpeed || speed > kMaxSpeed)
			return false;
		_speed = speed;
		return true;
	}

	std::unique_ptr<Composition> Composition::create()
	{
		return std::make_unique<CompositionImpl>();
	}

	std::unique_ptr<Composition> Composition::create(const char* source)
	{
		auto composition = std::make_unique<CompositionImpl>();
		composition->load(source);
		return composition;
	}
}
