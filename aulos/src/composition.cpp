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
		VoiceData* currentVoice = nullptr;
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

		const auto tryReadIdentifier = [&] {
			if (!((*source >= 'a' && *source <= 'z') || *source == '_'))
				return std::string_view{};
			const auto begin = source;
			do
				++source;
			while ((*source >= 'a' && *source <= 'z') || (*source >= '0' && *source <= '9') || *source == '_');
			const std::string_view result{ begin, static_cast<size_t>(source - begin) };
			skipSpaces();
			return result;
		};

		const auto readIdentifier = [&] {
			const auto result = tryReadIdentifier();
			if (result.empty())
				throw CompositionError{ location(), "Identifier expected" };
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
			if (!(*source >= '0' && *source <= '9') && *source != '-')
				return {};
			const auto begin = source;
			do
				++source;
			while (*source >= '0' && *source <= '9');
			if (*source == '.')
				do
					++source;
				while (*source >= '0' && *source <= '9');
#ifdef _MSC_VER
			float result;
			if (std::from_chars(begin, source, result).ec != std::errc{})
				throw CompositionError{ { line, begin - lineBase }, "Bad number" };
#else
			const auto result = std::strtof(std::string{ begin, source }.c_str(), nullptr);
#endif
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

		const auto parseNote = [&](std::vector<Sound>& sequence, size_t delay, size_t baseOffset) {
			assert(*source >= 'A' && *source <= 'G');
			assert(baseOffset < 12);
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
			++source;
		};

		const auto parseSequence = [&](std::vector<Sound>& sequence) {
			for (size_t delay = 0;;)
			{
				const auto mapNote = [&](char note) -> std::underlying_type_t<aulos::Note> {
					switch (note)
					{
					case 'A': return 9;
					case 'B': return 11;
					case 'C': return 0;
					case 'D': return 2;
					case 'E': return 4;
					case 'F': return 5;
					case 'G': return 7;
					default: throw CompositionError{ location(), "Bad note" };
					}
				};

				switch (*source)
				{
				case '\0':
					return;
				case '\r':
				case '\n':
					consumeEndOfLine();
					return;
				case ',':
					++delay;
					++source;
					break;
				default:
					parseNote(sequence, delay, mapNote(*source));
					delay = 0;
					break;
				}
			}
		};

		const auto parseCommand = [&](std::string_view command) {
			const auto readEnvelope = [&](Envelope& envelope, float minValue, float maxValue) {
				envelope._changes.clear();
				while (const auto duration = tryReadUnsigned(0, static_cast<unsigned>(EnvelopeChange::kMaxDuration.count())))
				{
					auto shape = EnvelopeShape::Linear;
					if (const auto shapeName = tryReadIdentifier(); !shapeName.empty())
					{
						if (shapeName == "smooth_quadratic_2")
							shape = EnvelopeShape::SmoothQuadratic2;
						else if (shapeName == "smooth_quadratic_4")
							shape = EnvelopeShape::SmoothQuadratic4;
						else if (shapeName == "sharp_quadratic_2")
							shape = EnvelopeShape::SharpQuadratic2;
						else if (shapeName == "sharp_quadratic_4")
							shape = EnvelopeShape::SharpQuadratic4;
						else
							throw CompositionError{ location(), "Unknown envelope shape" };
					}
					envelope._changes.emplace_back(std::chrono::milliseconds{ *duration }, readFloat(minValue, maxValue), shape);
				}
			};

			if (command == "amplitude")
			{
				if (currentSection != Section::Voice)
					throw CompositionError{ location(), "Unexpected command" };
				readEnvelope(currentVoice->_amplitudeEnvelope, 0.f, 1.f);
			}
			else if (command == "asymmetry")
			{
				if (currentSection != Section::Voice)
					throw CompositionError{ location(), "Unexpected command" };
				readEnvelope(currentVoice->_asymmetryEnvelope, 0.f, 1.f);
			}
			else if (command == "frequency")
			{
				if (currentSection != Section::Voice)
					throw CompositionError{ location(), "Unexpected command" };
				readEnvelope(currentVoice->_frequencyEnvelope, -1.f, 1.f);
			}
			else if (command == "oscillation")
			{
				if (currentSection != Section::Voice)
					throw CompositionError{ location(), "Unexpected command" };
				readEnvelope(currentVoice->_oscillationEnvelope, 0.f, 1.f);
			}
			else if (command == "stereo_delay")
			{
				if (currentSection != Section::Voice)
					throw CompositionError{ location(), "Unexpected command" };
				currentVoice->_stereoDelay = readFloat(-1'000.f, 1'000.f);
			}
			else if (command == "stereo_inversion")
			{
				if (currentSection != Section::Voice)
					throw CompositionError{ location(), "Unexpected command" };
				currentVoice->_stereoInversion = readUnsigned(0, 1) == 1;
			}
			else if (command == "stereo_pan")
			{
				if (currentSection != Section::Voice)
					throw CompositionError{ location(), "Unexpected command" };
				currentVoice->_stereoPan = readFloat(-1.f, 1.f);
			}
			else if (command == "wave")
			{
				if (currentSection != Section::Voice)
					throw CompositionError{ location(), "Unexpected command" };
				float minShape = 0;
				float maxShape = 0;
				if (const auto type = readIdentifier(); type == "linear")
					currentVoice->_waveShape = WaveShape::Linear;
				else if (type == "smooth_quadratic")
					currentVoice->_waveShape = WaveShape::SmoothQuadratic;
				else if (type == "sharp_quadratic")
					currentVoice->_waveShape = WaveShape::SharpQuadratic;
				else if (type == "cubic")
				{
					currentVoice->_waveShape = WaveShape::SmoothCubic;
					minShape = kMinSmoothCubicShape;
					maxShape = kMaxSmoothCubicShape;
				}
				else if (type == "quintic")
				{
					currentVoice->_waveShape = WaveShape::Quintic;
					minShape = kMinQuinticShape;
					maxShape = kMaxQuinticShape;
				}
				else if (type == "cosine")
					currentVoice->_waveShape = WaveShape::Cosine;
				else
					throw CompositionError{ location(), "Bad voice wave type" };
				if (const auto parameter = tryReadFloat(minShape, maxShape); parameter)
					currentVoice->_waveShapeParameter = *parameter;
				else
					currentVoice->_waveShapeParameter = 0;
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
			const auto roundedValue = std::lround(std::abs(value) * 100.f);
			auto result = std::to_string(roundedValue / 100);
			const auto remainder = roundedValue % 100;
			return (value < 0 ? "-" : "") + result + (remainder >= 10 ? '.' + std::to_string(remainder) : ".0" + std::to_string(remainder));
		};

		std::string text;
		if (!_author.empty())
			text += "\nauthor \"" + _author + '"';
		text += "\nspeed " + std::to_string(_speed);
		if (!_title.empty())
			text += "\ntitle \"" + _title + '"';
		for (const auto& part : _parts)
		{
			const auto saveEnvelope = [&text, &floatToString](std::string_view name, const Envelope& envelope) {
				if (envelope._changes.empty())
					return;
				text += '\n';
				text += name;
				for (const auto& change : envelope._changes)
				{
					text += ' ' + std::to_string(change._duration.count());
					switch (change._shape)
					{
					case EnvelopeShape::Linear: break;
					case EnvelopeShape::SmoothQuadratic2: text += " smooth_quadratic_2"; break;
					case EnvelopeShape::SmoothQuadratic4: text += " smooth_quadratic_4"; break;
					case EnvelopeShape::SharpQuadratic2: text += " sharp_quadratic_2"; break;
					case EnvelopeShape::SharpQuadratic4: text += " sharp_quadratic_4"; break;
					}
					text += ' ' + floatToString(change._value);
				}
			};

			const auto partIndex = static_cast<size_t>(&part - _parts.data() + 1);
			text += "\n\n@voice " + std::to_string(partIndex);
			if (!part._voiceName.empty())
				text += " \"" + part._voiceName + '"';
			saveEnvelope("amplitude", part._voice._amplitudeEnvelope);
			saveEnvelope("asymmetry", part._voice._asymmetryEnvelope);
			saveEnvelope("frequency", part._voice._frequencyEnvelope);
			saveEnvelope("oscillation", part._voice._oscillationEnvelope);
			text += "\nstereo_delay " + floatToString(part._voice._stereoDelay);
			text += "\nstereo_inversion " + std::to_string(static_cast<int>(part._voice._stereoInversion));
			text += "\nstereo_pan " + floatToString(part._voice._stereoPan);
			text += "\nwave ";
			switch (part._voice._waveShape)
			{
			case WaveShape::Linear: text += "linear"; break;
			case WaveShape::SmoothQuadratic: text += "smooth_quadratic"; break;
			case WaveShape::SharpQuadratic: text += "sharp_quadratic"; break;
			case WaveShape::SmoothCubic: text += "cubic " + floatToString(part._voice._waveShapeParameter); break;
			case WaveShape::Quintic: text += "quintic " + floatToString(part._voice._waveShapeParameter); break;
			case WaveShape::Cosine: text += "cosine"; break;
			}
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
					text += '\n' + std::to_string(partIndex) + ' ' + std::to_string(trackIndex) + ' ' + std::to_string(sequenceIndex) + ' ';
					for (const auto& sound : sequence)
					{
						text.append(sound._delay, ',');
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
			auto& partData = _parts.emplace_back(std::make_shared<PartData>(std::make_shared<VoiceData>(packedPart._voice)));
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
					packedTrack._fragments.emplace_back(fragmentData.first - lastOffset, static_cast<size_t>(s - trackData->_sequences.begin()));
					lastOffset = fragmentData.first;
				}
			}
		}
		packed->_title = _title;
		packed->_author = _author;
		return packed;
	}
}
