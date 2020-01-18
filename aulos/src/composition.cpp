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

#include "composition.hpp"

#include <algorithm>
#include <cassert>
#include <charconv>
#include <limits>
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
			Tracks,
			Sequences,
			Fragments,
		};

		size_t line = 1;
		const char* lineBase = source;
		Section currentSection = Section::Initial;
		Voice* currentVoice = nullptr;
		Track* currentTrack = nullptr;

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

		const auto tryReadString = [&]() -> std::optional<std::string> {
			if (*source != '"')
				return {};
			const auto begin = ++source;
			while (*source && *source != '"')
				++source;
			if (!*source)
				throw CompositionError{ { line, begin - lineBase }, "Unexpected end of file" };
			const auto end = source++;
			skipSpaces();
			return std::string{ begin, end };
		};

		const auto parseNote = [&](std::vector<Sound>& sequence, size_t baseOffset) {
			assert(*source >= 'A' && *source <= 'G');
			assert(baseOffset >= 0 && baseOffset < 12);
			++source;
			if (*source == '#')
			{
				if (baseOffset == 11)
					throw CompositionError{ location(), "Note overflow" };
				++baseOffset;
				++source;
			}
			else if (*source == 'b')
			{
				if (!baseOffset)
					throw CompositionError{ location(), "Note underflow" };
				--baseOffset;
				++source;
			}
			if (*source < '0' || *source > '9')
				throw CompositionError{ location(), "Bad note" };
			sequence.emplace_back(static_cast<Note>((*source - '0') * 12 + baseOffset), 1);
			++source;
		};

		const auto parseSequence = [&](std::vector<Sound>& sequence) {
			for (;;)
			{
				switch (*source)
				{
				case '\0':
					return false;
				case '\r':
				case '\n':
					consumeEndOfLine();
					return true;
				case '\t':
				case ' ':
					break;
				case '.':
					if (sequence.empty())
						throw CompositionError{ location(), "Bad note" };
					++source;
					++sequence.back()._pause;
					break;
				case 'A':
					parseNote(sequence, 9);
					break;
				case 'B':
					parseNote(sequence, 11);
					break;
				case 'C':
					parseNote(sequence, 0);
					break;
				case 'D':
					parseNote(sequence, 2);
					break;
				case 'E':
					parseNote(sequence, 4);
					break;
				case 'F':
					parseNote(sequence, 5);
					break;
				case 'G':
					parseNote(sequence, 7);
					break;
				default:
					throw CompositionError{ location(), "Bad note" };
				}
				skipSpaces();
			}
		};

		const auto parseCommand = [&](std::string_view command) {
			if (command == "amplitude")
			{
				if (currentSection != Section::Voice)
					throw CompositionError{ location(), "Unexpected command" };
				auto& envelope = currentVoice->_amplitudeEnvelope;
				envelope._initial = readFloat(0.f, 1.f);
				envelope._changes.clear();
				while (const auto delay = tryReadFloat(0.f, kMaxEnvelopePartDuration))
					envelope._changes.emplace_back(*delay, readFloat(0.f, 1.f));
			}
			else if (command == "asymmetry")
			{
				if (currentSection != Section::Voice)
					throw CompositionError{ location(), "Unexpected command" };
				auto& envelope = currentVoice->_asymmetryEnvelope;
				envelope._initial = readFloat(0.f, 1.f);
				envelope._changes.clear();
				while (const auto delay = tryReadFloat(0.f, kMaxEnvelopePartDuration))
					envelope._changes.emplace_back(*delay, readFloat(0.f, 1.f));
			}
			else if (command == "frequency")
			{
				if (currentSection != Section::Voice)
					throw CompositionError{ location(), "Unexpected command" };
				constexpr auto minFrequency = std::numeric_limits<float>::min();
				auto& envelope = currentVoice->_frequencyEnvelope;
				envelope._initial = readFloat(minFrequency, 1.f);
				while (const auto delay = tryReadFloat(0.f, kMaxEnvelopePartDuration))
					envelope._changes.emplace_back(*delay, readFloat(minFrequency, 1.f));
			}
			else if (command == "wave")
			{
				if (currentSection != Section::Voice)
					throw CompositionError{ location(), "Unexpected command" };
				if (const auto type = readIdentifier(); type == "linear")
				{
					const auto oscillation = tryReadFloat(0.f, 1.f);
					currentVoice->_wave = Wave::Linear;
					currentVoice->_oscillation = oscillation ? *oscillation : 1.f;
				}
				else
					throw CompositionError{ location(), "Bad voice wave type" };
			}
			else if (command == "speed")
			{
				if (currentSection != Section::Initial)
					throw CompositionError{ location(), "Unexpected command" };
				_speed = readUnsigned(kMinSpeed, kMaxSpeed);
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
					const auto trackIndex = readUnsigned(1, static_cast<unsigned>(_tracks.size()));
					const auto sequenceIndex = static_cast<unsigned>(_tracks[trackIndex - 1]._sequences.size() + 1);
					readUnsigned(sequenceIndex, sequenceIndex);
					parseSequence(_tracks[trackIndex - 1]._sequences.emplace_back());
					break;
				}
				case Section::Tracks: {
					const auto trackIndex = static_cast<unsigned>(_tracks.size() + 1);
					readUnsigned(trackIndex, trackIndex);
					const auto voiceIndex = readUnsigned(1, static_cast<unsigned>(_voices.size()));
					const auto weight = tryReadUnsigned(1, 256);
					_tracks.emplace_back(voiceIndex - 1, weight ? *weight : 1);
					break;
				}
				case Section::Fragments: {
					assert(currentTrack);
					const auto delay = readUnsigned(0, std::numeric_limits<unsigned>::max());
					const auto sequenceIndex = readUnsigned(1, static_cast<unsigned>(currentTrack->_sequences.size()));
					consumeEndOfLine();
					currentTrack->_fragments.emplace_back(delay, sequenceIndex - 1);
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
					auto name = tryReadString();
					consumeEndOfLine();
					currentSection = Section::Voice;
					currentVoice = &_voices.emplace_back();
					if (name)
						currentVoice->_name = std::move(*name);
				}
				else if (section == "tracks")
				{
					consumeEndOfLine();
					currentSection = Section::Tracks;
				}
				else if (section == "sequences")
				{
					consumeEndOfLine();
					currentSection = Section::Sequences;
				}
				else if (section == "fragments")
				{
					const auto trackIndex = readUnsigned(1, static_cast<unsigned>(_tracks.size()));
					consumeEndOfLine();
					currentSection = Section::Fragments;
					currentTrack = &_tracks[trackIndex - 1];
				}
				else
					throw CompositionError{ location(), "Unknown section \"@" + std::string{ section } + "\"" };
				break;
			default:
				parseCommand(readIdentifier());
			}
		}
	}

	std::unique_ptr<Composition> Composition::create(const char* textSource)
	{
		auto composition = std::make_unique<CompositionImpl>();
		composition->load(textSource);
		return composition;
	}

	CompositionData::CompositionData(const Composition& composition)
	{
		const auto& packed = static_cast<const CompositionImpl&>(composition);
		_speed = packed._speed;
		_voices.reserve(packed._voices.size());
		for (const auto& packedVoice : packed._voices)
			_voices.emplace_back(std::make_unique<Voice>(packedVoice));
		_tracks.reserve(packed._tracks.size());
		for (const auto& packedTrack : packed._tracks)
		{
			auto& trackData = _tracks.emplace_back(_voices[packedTrack._voice], packedTrack._weight);
			trackData._sequences.reserve(packedTrack._sequences.size());
			for (const auto& packedSequence : packedTrack._sequences)
				trackData._sequences.emplace_back(std::make_shared<SequenceData>())->_sounds = packedSequence;
			size_t offset = 0;
			for (const auto& packedFragment : packedTrack._fragments)
				trackData._fragments.insert_or_assign(offset += packedFragment._delay, trackData._sequences[packedFragment._sequence]);
		}
	}

	std::unique_ptr<Composition> CompositionData::pack() const
	{
		// TODO: Verify values.
		auto packed = std::make_unique<CompositionImpl>();
		packed->_speed = _speed;
		packed->_voices.reserve(_voices.size());
		for (const auto& voiceData : _voices)
			packed->_voices.emplace_back(*voiceData);
		packed->_tracks.reserve(_tracks.size());
		for (const auto& trackData : _tracks)
		{
			const auto v = std::find(_voices.begin(), _voices.end(), trackData._voice);
			if (v == _voices.end())
				return {};
			auto& packedTrack = packed->_tracks.emplace_back(v - _voices.begin(), trackData._weight);
			packedTrack._sequences.reserve(trackData._sequences.size());
			for (const auto& sequenceData : trackData._sequences)
				packedTrack._sequences.emplace_back(sequenceData->_sounds);
			packedTrack._fragments.reserve(trackData._fragments.size());
			size_t lastOffset = 0;
			for (const auto& fragmentData : trackData._fragments)
			{
				const auto s = std::find(trackData._sequences.begin(), trackData._sequences.end(), fragmentData.second);
				if (s == trackData._sequences.end())
					return {};
				packedTrack._fragments.emplace_back(fragmentData.first - lastOffset, s - trackData._sequences.begin());
				lastOffset = fragmentData.first;
			}
		}
		return packed;
	}
}
