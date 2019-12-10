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
	constexpr float kMinSpeed = 1.f;
	constexpr float kMaxSpeed = 32.f;

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
	CompositionImpl::CompositionImpl()
		: _speed{ kMinSpeed }
	{
	}

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

		const auto tryReadUnsigned = [&](unsigned min, unsigned max) -> std::optional<unsigned> {
			if (*source < '0' || *source > '9')
				return {};
			const auto begin = source;
			do
				++source;
			while (*source >= '0' && *source <= '9');
			unsigned result;
			if (std::from_chars(begin, source, result).ec != std::errc{})
				throw CompositionError{ { line, begin - lineBase }, "Number expected" };
			if (result < min || result > max)
				throw CompositionError{ { line, begin - lineBase }, "Number is out of range" };
			skipSpaces();
			return result;
		};

		const auto readUnsigned = [&](unsigned min, unsigned max) {
			const auto result = tryReadUnsigned(min, max);
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

		const auto parseNote = [&](std::vector<Sound>& sequence, const char* data, size_t baseOffset) {
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
			sequence.emplace_back(static_cast<Note>((*data - '0') * 12 + baseOffset));
			return data + 1;
		};

		const auto parseNotes = [&](std::vector<Sound>& sequence) {
			for (;;)
			{
				switch (*source++)
				{
				case '\0':
					return false;
				case '\r':
				case '\n':
					consumeEndOfLine();
					return true;
				case '\t':
				case ' ':
					while (*source == ' ' || *source == '\t')
						++source;
					break;
				case '.':
					if (sequence.empty())
						sequence.emplace_back(Note::Silence);
					else
						++sequence.back()._duration;
					break;
				case 'A':
					source = parseNote(sequence, source, 9);
					break;
				case 'B':
					source = parseNote(sequence, source, 11);
					break;
				case 'C':
					source = parseNote(sequence, source, 0);
					break;
				case 'D':
					source = parseNote(sequence, source, 2);
					break;
				case 'E':
					source = parseNote(sequence, source, 4);
					break;
				case 'F':
					source = parseNote(sequence, source, 5);
					break;
				case 'G':
					source = parseNote(sequence, source, 7);
					break;
				default:
					throw CompositionError{ location(), "Bad note" };
				}
			}
		};

		const auto parseCommand = [&](std::string_view command) {
			constexpr auto kMaxEnvelopePartDuration = 60.f;
			if (command == "amplitude")
			{
				if (currentSection != Section::Voice)
					throw CompositionError{ location(), "Unexpected command" };
				if (const auto type = readIdentifier(); type == "adsr")
				{
					const auto attack = readFloat(0.f, kMaxEnvelopePartDuration);
					const auto decay = readFloat(0.f, kMaxEnvelopePartDuration);
					const auto sustain = readFloat(0.f, 1.f);
					const auto release = readFloat(0.f, kMaxEnvelopePartDuration);
					currentVoice->_amplitude._points = { { attack, 1.f }, { decay, sustain }, { release, 0.f } };
				}
				else if (type == "ahdsr")
				{
					const auto attack = readFloat(0.f, kMaxEnvelopePartDuration);
					const auto hold = readFloat(0.f, kMaxEnvelopePartDuration);
					const auto decay = readFloat(0.f, kMaxEnvelopePartDuration);
					const auto sustain = readFloat(0.f, 1.f);
					const auto release = readFloat(0.f, kMaxEnvelopePartDuration);
					currentVoice->_amplitude._points = { { attack, 1.f }, { hold, 1.f }, { decay, sustain }, { release, 0.f } };
				}
				else
					throw CompositionError{ location(), "Bad envelope type" };
			}
			else if (command == "asymmetry")
			{
				if (currentSection != Section::Voice)
					throw CompositionError{ location(), "Unexpected command" };
				std::vector<EnvelopeData::Point> points;
				points.emplace_back(0.f, readFloat(0.f, 1.f));
				while (const auto delay = tryReadFloat(0.f, kMaxEnvelopePartDuration))
					points.emplace_back(*delay, readFloat(0.f, 1.f));
				currentVoice->_asymmetry._points = std::move(points);
			}
			else if (command == "frequency")
			{
				if (currentSection != Section::Voice)
					throw CompositionError{ location(), "Unexpected command" };
				constexpr auto minFrequency = std::numeric_limits<float>::min();
				std::vector<EnvelopeData::Point> points;
				points.emplace_back(0.f, readFloat(minFrequency, 1.f));
				while (const auto delay = tryReadFloat(0.f, kMaxEnvelopePartDuration))
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
				currentVoice->_weight = readUnsigned(1, 256);
				consumeEndOfLine();
			}
			else if (command == "speed")
			{
				if (currentSection != Section::Initial)
					throw CompositionError{ location(), "Unexpected command" };
				_speed = readFloat(kMinSpeed, kMaxSpeed);
				consumeEndOfLine();
			}
			else
				throw CompositionError{ location(), "Unknown command \"" + std::string{ command } + "\"" };
			if (*source && *source != '\n' && *source != '\r')
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
				case Section::Sequences: {
					const auto sequenceIndex = static_cast<unsigned>(_sequences.size() + 1);
					readUnsigned(sequenceIndex, sequenceIndex);
					parseNotes(_sequences.emplace_back());
					break;
				}
				case Section::Fragments: {
					const auto nextFragmentDelay = readUnsigned(0, std::numeric_limits<unsigned>::max());
					while (const auto trackIndex = tryReadUnsigned(1, static_cast<unsigned>(_voices.size())))
					{
						const auto sequenceIndex = readUnsigned(1, static_cast<unsigned>(_sequences.size()));
						_fragments.emplace_back(std::exchange(lastFragmentDelay, 0), *trackIndex - 1, sequenceIndex - 1);
					}
					lastFragmentDelay += nextFragmentDelay;
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
					const auto voiceIndex = static_cast<unsigned>(_voices.size() + 1);
					readUnsigned(voiceIndex, voiceIndex);
					consumeEndOfLine();
					currentSection = Section::Voice;
					currentVoice = &_voices.emplace_back();
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

	Sequence CompositionImpl::sequence(size_t index) const noexcept
	{
		if (index >= _sequences.size())
			return {};
		auto& value = _sequences[index];
		return { value.data(), value.size() };
	}

	std::unique_ptr<Composition> Composition::create(const char* textSource)
	{
		auto composition = std::make_unique<CompositionImpl>();
		composition->load(textSource);
		return composition;
	}
}
