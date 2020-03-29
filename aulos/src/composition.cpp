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
#include <cmath>
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
			Global,
			Voice,
			Tracks,
			Sequences,
			Fragments,
		};

		size_t line = 1;
		const char* lineBase = source;
		Section currentSection = Section::Global;
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
			if (!(*source >= 'a' && *source <= 'z' || *source >= 'A' && *source <= 'Z'))
				throw CompositionError{ location(), "Identifier expected" };
			const auto begin = source;
			do
				++source;
			while (*source >= 'a' && *source <= 'z' || *source >= 'A' && *source <= 'Z');
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

		const auto readString = [&] {
			const auto result = tryReadString();
			if (!result)
				throw CompositionError{ location(), "String expected" };
			return *result;
		};

		const auto parseNote = [&](std::vector<Sound>& sequence, size_t& delay, size_t baseOffset) {
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
			sequence.emplace_back(delay, static_cast<Note>((*source - '0') * 12 + baseOffset));
			delay = 1;
			++source;
		};

		const auto parseSequence = [&](std::vector<Sound>& sequence) {
			for (size_t delay = 0;;)
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
					++delay;
					++source;
					break;
				case 'A':
					parseNote(sequence, delay, 9);
					break;
				case 'B':
					parseNote(sequence, delay, 11);
					break;
				case 'C':
					parseNote(sequence, delay, 0);
					break;
				case 'D':
					parseNote(sequence, delay, 2);
					break;
				case 'E':
					parseNote(sequence, delay, 4);
					break;
				case 'F':
					parseNote(sequence, delay, 5);
					break;
				case 'G':
					parseNote(sequence, delay, 7);
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
				if (const auto type = readIdentifier(); type == "Linear")
				{
					const auto oscillation = tryReadFloat(0.f, 1.f);
					currentVoice->_wave = Wave::Linear;
					currentVoice->_oscillation = oscillation ? *oscillation : 1.f;
				}
				else if (type == "Quadratic")
				{
					const auto oscillation = tryReadFloat(0.f, 1.f);
					currentVoice->_wave = Wave::Quadratic;
					currentVoice->_oscillation = oscillation ? *oscillation : 1.f;
				}
				else if (type == "Cubic")
				{
					const auto oscillation = tryReadFloat(0.f, 1.f);
					currentVoice->_wave = Wave::Cubic;
					currentVoice->_oscillation = oscillation ? *oscillation : 1.f;
				}
				else if (type == "Cosine")
				{
					const auto oscillation = tryReadFloat(0.f, 1.f);
					currentVoice->_wave = Wave::Cosine;
					currentVoice->_oscillation = oscillation ? *oscillation : 1.f;
				}
				else
					throw CompositionError{ location(), "Bad voice wave type" };
			}
			else if (command == "speed")
			{
				if (currentSection != Section::Global)
					throw CompositionError{ location(), "Unexpected command" };
				_speed = readUnsigned(kMinSpeed, kMaxSpeed);
			}
			else if (command == "title")
			{
				if (currentSection != Section::Global)
					throw CompositionError{ location(), "Unexpected command" };
				_title = readString();
			}
			else if (command == "author")
			{
				if (currentSection != Section::Global)
					throw CompositionError{ location(), "Unexpected command" };
				_author = readString();
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
					auto& part = _parts[readUnsigned(1, static_cast<unsigned>(_parts.size())) - 1];
					auto& track = part._tracks[readUnsigned(1, static_cast<unsigned>(part._tracks.size())) - 1];
					const auto sequenceIndex = static_cast<unsigned>(track._sequences.size() + 1);
					readUnsigned(sequenceIndex, sequenceIndex);
					parseSequence(track._sequences.emplace_back());
					break;
				}
				case Section::Tracks: {
					auto& part = _parts[readUnsigned(1, static_cast<unsigned>(_parts.size())) - 1];
					const auto trackIndex = static_cast<unsigned>(part._tracks.size() + 1);
					readUnsigned(trackIndex, trackIndex);
					const auto weight = tryReadUnsigned(1, 255);
					part._tracks.emplace_back(weight ? *weight : 1);
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
			case '@':
				++source;
				if (const auto section = readIdentifier(); section == "voice")
				{
					const auto partIndex = static_cast<unsigned>(_parts.size() + 1);
					readUnsigned(partIndex, partIndex);
					auto name = tryReadString();
					consumeEndOfLine();
					currentSection = Section::Voice;
					currentVoice = &_parts.emplace_back()._voice;
					if (name)
						_parts.back()._voiceName = std::move(*name);
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
					auto& part = _parts[readUnsigned(1, static_cast<unsigned>(_parts.size())) - 1];
					auto& track = part._tracks[readUnsigned(1, static_cast<unsigned>(part._tracks.size())) - 1];
					consumeEndOfLine();
					currentSection = Section::Fragments;
					currentTrack = &track;
				}
				else
					throw CompositionError{ location(), "Unknown section \"@" + std::string{ section } + "\"" };
				break;
			default:
				parseCommand(readIdentifier());
			}
		}
	}

	std::vector<std::byte> CompositionImpl::save() const
	{
		const auto floatToString = [](float value) {
			const auto roundedValue = std::lround(value * 100.f);
			auto result = std::to_string(roundedValue / 100);
			const auto remainder = roundedValue % 100;
			return result + (remainder >= 10 ? '.' + std::to_string(remainder) : ".0" + std::to_string(remainder));
		};

		std::string text;
		if (!_author.empty())
			text += "\nauthor \"" + _author + '"';
		text += "\nspeed " + std::to_string(_speed);
		if (!_title.empty())
			text += "\ntitle \"" + _title + '"';
		for (const auto& part : _parts)
		{
			const auto partIndex = static_cast<size_t>(&part - _parts.data() + 1);
			text += "\n\n@voice " + std::to_string(partIndex);
			if (!part._voiceName.empty())
				text += " \"" + part._voiceName + '"';
			text += "\nwave ";
			switch (part._voice._wave)
			{
			case Wave::Linear: text += "Linear"; break;
			case Wave::Quadratic: text += "Quadratic"; break;
			case Wave::Cubic: text += "Cubic"; break;
			case Wave::Cosine: text += "Cosine"; break;
			}
			text += ' ' + floatToString(part._voice._oscillation);
			text += "\namplitude " + floatToString(part._voice._amplitudeEnvelope._initial);
			for (const auto& change : part._voice._amplitudeEnvelope._changes)
				text += ' ' + floatToString(change._delay) + ' ' + floatToString(change._value);
			text += "\nfrequency " + floatToString(part._voice._frequencyEnvelope._initial);
			for (const auto& change : part._voice._frequencyEnvelope._changes)
				text += ' ' + floatToString(change._delay) + ' ' + floatToString(change._value);
			text += "\nasymmetry " + floatToString(part._voice._asymmetryEnvelope._initial);
			for (const auto& change : part._voice._asymmetryEnvelope._changes)
				text += ' ' + floatToString(change._delay) + ' ' + floatToString(change._value);
		}
		text += "\n\n@tracks";
		for (const auto& part : _parts)
		{
			const auto partIndex = static_cast<size_t>(&part - _parts.data() + 1);
			for (const auto& track : part._tracks)
			{
				const auto trackIndex = static_cast<size_t>(&track - part._tracks.data() + 1);
				text += '\n' + std::to_string(partIndex) + ' ' + std::to_string(trackIndex) + ' ' + std::to_string(track._weight);
			}
		}
		text += "\n\n@sequences";
		for (const auto& part : _parts)
		{
			const auto partIndex = static_cast<size_t>(&part - _parts.data() + 1);
			for (const auto& track : part._tracks)
			{
				const auto trackIndex = static_cast<size_t>(&track - part._tracks.data() + 1);
				for (const auto& sequence : track._sequences)
				{
					const auto sequenceIndex = static_cast<size_t>(&sequence - track._sequences.data() + 1);
					text += '\n' + std::to_string(partIndex) + ' ' + std::to_string(trackIndex) + ' ' + std::to_string(sequenceIndex);
					for (const auto& sound : sequence)
					{
						for (auto i = sound._delay; i > 1; --i)
							text += " .";
						text += ' ';
						const auto note = static_cast<unsigned>(sound._note);
						switch (note % 12)
						{
						case 0: text += "C"; break;
						case 1: text += "C#"; break;
						case 2: text += "D"; break;
						case 3: text += "D#"; break;
						case 4: text += "E"; break;
						case 5: text += "F"; break;
						case 6: text += "F#"; break;
						case 7: text += "G"; break;
						case 8: text += "G#"; break;
						case 9: text += "A"; break;
						case 10: text += "A#"; break;
						case 11: text += "B"; break;
						}
						text += std::to_string(note / 12);
					}
				}
			}
		}
		for (const auto& part : _parts)
		{
			const auto partIndex = static_cast<size_t>(&part - _parts.data() + 1);
			for (const auto& track : part._tracks)
			{
				if (track._fragments.empty())
					continue;
				const auto trackIndex = static_cast<size_t>(&track - part._tracks.data() + 1);
				text += "\n\n@fragments " + std::to_string(partIndex) + ' ' + std::to_string(trackIndex);
				for (const auto& fragment : track._fragments)
					text += '\n' + std::to_string(fragment._delay) + ' ' + std::to_string(fragment._sequence + 1);
			}
		}
		text += '\n';

		std::vector<std::byte> buffer;
		buffer.reserve(text.size());
		std::for_each(std::next(text.begin()), text.end(), [&buffer](const auto c) { buffer.emplace_back(static_cast<std::byte>(c)); });
		return buffer;
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
		_parts.reserve(packed._parts.size());
		for (const auto& packedPart : packed._parts)
		{
			auto& partData = _parts.emplace_back(std::make_shared<PartData>(std::make_shared<Voice>(packedPart._voice)));
			partData->_voiceName = packedPart._voiceName;
			partData->_tracks.reserve(packedPart._tracks.size());
			for (const auto& packedTrack : packedPart._tracks)
			{
				auto& trackData = *partData->_tracks.emplace_back(std::make_shared<TrackData>(packedTrack._weight));
				trackData._sequences.reserve(packedTrack._sequences.size());
				for (const auto& packedSequence : packedTrack._sequences)
					trackData._sequences.emplace_back(std::make_shared<SequenceData>())->_sounds = packedSequence;
				size_t offset = 0;
				for (const auto& packedFragment : packedTrack._fragments)
					trackData._fragments.insert_or_assign(offset += packedFragment._delay, trackData._sequences[packedFragment._sequence]);
			}
		}
		_title = packed._title;
		_author = packed._author;
	}

	std::unique_ptr<Composition> CompositionData::pack() const
	{
		auto packed = std::make_unique<CompositionImpl>();
		packed->_speed = _speed;
		packed->_parts.reserve(_parts.size());
		for (const auto& partData : _parts)
		{
			auto& packedPart = packed->_parts.emplace_back();
			packedPart._voice = *partData->_voice;
			packedPart._voiceName = partData->_voiceName;
			packedPart._tracks.reserve(partData->_tracks.size());
			for (const auto& trackData : partData->_tracks)
			{
				auto& packedTrack = packedPart._tracks.emplace_back(trackData->_weight);
				packedTrack._sequences.reserve(trackData->_sequences.size());
				for (const auto& sequenceData : trackData->_sequences)
					packedTrack._sequences.emplace_back(sequenceData->_sounds);
				packedTrack._fragments.reserve(trackData->_fragments.size());
				size_t lastOffset = 0;
				for (const auto& fragmentData : trackData->_fragments)
				{
					const auto s = std::find(trackData->_sequences.begin(), trackData->_sequences.end(), fragmentData.second);
					if (s == trackData->_sequences.end())
						return {};
					packedTrack._fragments.emplace_back(fragmentData.first - lastOffset, s - trackData->_sequences.begin());
					lastOffset = fragmentData.first;
				}
			}
		}
		packed->_title = _title;
		packed->_author = _author;
		return packed;
	}
}
