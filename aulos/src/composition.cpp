// This file is part of the Aulos toolkit.
// Copyright (C) Sergei Blagodarin.
// SPDX-License-Identifier: Apache-2.0

#include "composition.hpp"

#include "shaper.hpp"

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
			else if (command == "loop")
			{
				if (currentSection != Section::Global)
					throw CompositionError{ location(), "Unexpected command" };
				_loopOffset = readUnsigned(0, std::numeric_limits<unsigned>::max());
				_loopLength = readUnsigned(0, std::numeric_limits<unsigned>::max());
			}
			else if (command == "oscillation")
			{
				if (currentSection != Section::Voice)
					throw CompositionError{ location(), "Unexpected command" };
				readEnvelope(currentVoice->_oscillationEnvelope, 0.f, 1.f);
			}
			else if (command == "polyphony")
			{
				if (currentSection != Section::Voice)
					throw CompositionError{ location(), "Unexpected command" };
				if (const auto type = readIdentifier(); type == "chord")
					currentVoice->_polyphony = Polyphony::Chord;
				else if (type == "full")
					currentVoice->_polyphony = Polyphony::Full;
				else
					throw CompositionError{ location(), "Bad voice polyphony" };
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
			else if (command == "stereo_radius")
			{
				if (currentSection != Section::Voice)
					throw CompositionError{ location(), "Unexpected command" };
				currentVoice->_stereoRadius = readFloat(-1'000.f, 1'000.f);
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
					currentVoice->_waveShape = WaveShape::Cubic;
					minShape = CubicShaper::kMinShape;
					maxShape = CubicShaper::kMaxShape;
				}
				else if (type == "quintic")
				{
					currentVoice->_waveShape = WaveShape::Quintic;
					minShape = QuinticShaper::kMinShape;
					maxShape = QuinticShaper::kMaxShape;
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
					auto& part = _parts[readUnsigned(1, static_cast<unsigned>(_parts.size())) - 1];
					auto& track = part._tracks[readUnsigned(1, static_cast<unsigned>(part._tracks.size())) - 1];
					while (const auto delay = tryReadUnsigned(0, std::numeric_limits<unsigned>::max()))
						track._fragments.emplace_back(*delay, readUnsigned(1, static_cast<unsigned>(track._sequences.size())) - 1);
					consumeEndOfLine();
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

	std::unique_ptr<Composition> Composition::create(const char* textSource)
	{
		auto composition = std::make_unique<CompositionImpl>();
		composition->load(textSource);
		return composition;
	}
}
