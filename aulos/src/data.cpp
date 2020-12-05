// This file is part of the Aulos toolkit.
// Copyright (C) Sergei Blagodarin.
// SPDX-License-Identifier: Apache-2.0

#include <aulos/data.hpp>

#include "composition.hpp"

#include <algorithm>
#include <cmath>

namespace aulos
{
	CompositionData::CompositionData(const Composition& composition)
	{
		const auto& packed = static_cast<const CompositionImpl&>(composition);
		_speed = packed._speed;
		_loopOffset = packed._loopOffset;
		_loopLength = packed._loopLength;
		_gainDivisor = static_cast<float>(packed._gainDivisor);
		_parts.reserve(packed._parts.size());
		for (const auto& packedPart : packed._parts)
		{
			auto& partData = _parts.emplace_back(std::make_shared<PartData>(std::make_shared<VoiceData>(packedPart._voice)));
			partData->_voiceName = packedPart._voiceName;
			partData->_tracks.reserve(packedPart._tracks.size());
			for (const auto& packedTrack : packedPart._tracks)
			{
				auto& trackData = *partData->_tracks.emplace_back(std::make_shared<TrackData>(std::make_shared<TrackProperties>(packedTrack._properties)));
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

	CompositionData::CompositionData(const std::shared_ptr<VoiceData>& voice, Note note)
	{
		const auto sequence = std::make_shared<SequenceData>();
		sequence->_sounds.emplace_back(0u, note);
		const auto track = std::make_shared<TrackData>(std::make_shared<TrackProperties>());
		track->_sequences.emplace_back(sequence);
		track->_fragments.emplace(0u, sequence);
		const auto part = std::make_shared<PartData>(voice);
		part->_tracks.emplace_back(track);
		_parts.emplace_back(part);
	}

	std::unique_ptr<Composition> CompositionData::pack() const
	{
		auto packed = std::make_unique<CompositionImpl>();
		packed->_speed = _speed;
		packed->_loopOffset = _loopOffset;
		packed->_loopLength = _loopLength;
		packed->_gainDivisor = decltype(packed->_gainDivisor)::ceil(_gainDivisor);
		packed->_parts.reserve(_parts.size());
		std::vector<std::shared_ptr<SequenceData>> currentTrackSequences;
		for (const auto& partData : _parts)
		{
			auto& packedPart = packed->_parts.emplace_back();
			packedPart._voice = *partData->_voice;
			packedPart._voiceName = partData->_voiceName;
			packedPart._tracks.reserve(partData->_tracks.size());
			for (const auto& trackData : partData->_tracks)
			{
				auto& packedTrack = packedPart._tracks.emplace_back();
				packedTrack._properties = *trackData->_properties;
				packedTrack._fragments.reserve(trackData->_fragments.size());
				for (size_t lastOffset = 0; const auto& fragmentData : trackData->_fragments)
				{
					if (fragmentData.second->_sounds.empty())
						continue;
					size_t sequenceIndex = 0;
					if (auto i = std::find(currentTrackSequences.cbegin(), currentTrackSequences.cend(), fragmentData.second); i == currentTrackSequences.cend())
					{
						sequenceIndex = currentTrackSequences.size();
						currentTrackSequences.emplace_back(fragmentData.second);
					}
					else
						sequenceIndex = static_cast<size_t>(i - currentTrackSequences.cbegin());
					packedTrack._fragments.emplace_back(fragmentData.first - lastOffset, sequenceIndex);
					lastOffset = fragmentData.first;
				}
				packedTrack._sequences.reserve(currentTrackSequences.size());
				for (const auto& sequenceData : currentTrackSequences)
					packedTrack._sequences.emplace_back(sequenceData->_sounds);
				currentTrackSequences.clear();
			}
		}
		packed->_title = _title;
		packed->_author = _author;
		return packed;
	}

	std::vector<std::byte> serialize(const Composition& composition)
	{
		const auto floatToString = [](float value) {
			const auto roundedValue = std::lround(std::abs(value) * 100.f);
			auto result = std::to_string(roundedValue / 100);
			const auto remainder = roundedValue % 100;
			return (value < 0 ? "-" : "") + result + (remainder >= 10 ? '.' + std::to_string(remainder) : ".0" + std::to_string(remainder));
		};

		const auto& impl = static_cast<const CompositionImpl&>(composition);
		std::string text;
		if (!impl._author.empty())
			text += "\nauthor \"" + impl._author + '"';
		text += "\ngain " + std::to_string(impl._gainDivisor.store());
		if (impl._loopLength > 0)
			text += "\nloop " + std::to_string(impl._loopOffset) + " " + std::to_string(impl._loopLength);
		text += "\nspeed " + std::to_string(impl._speed);
		if (!impl._title.empty())
			text += "\ntitle \"" + impl._title + '"';
		for (const auto& part : impl._parts)
		{
			const auto saveEnvelope = [&text, &floatToString](std::string_view name, const Envelope& envelope) {
				if (envelope._changes.empty())
					return;
				text += '\n';
				text += name;
				for (const auto& change : envelope._changes)
					text += ' ' + std::to_string(change._duration.count()) + ' ' + floatToString(change._value);
			};

			const auto partIndex = static_cast<size_t>(&part - impl._parts.data() + 1);
			text += "\n\n@voice " + std::to_string(partIndex);
			if (!part._voiceName.empty())
				text += " \"" + part._voiceName + '"';
			saveEnvelope("amplitude", part._voice._amplitudeEnvelope);
			saveEnvelope("asymmetry", part._voice._asymmetryEnvelope);
			saveEnvelope("frequency", part._voice._frequencyEnvelope);
			saveEnvelope("oscillation", part._voice._oscillationEnvelope);
			text += "\nwave ";
			switch (part._voice._waveShape)
			{
			case WaveShape::Linear: text += "linear"; break;
			case WaveShape::Quadratic: text += "quadratic " + floatToString(part._voice._waveShapeParameter); break;
			case WaveShape::Cubic: text += "cubic " + floatToString(part._voice._waveShapeParameter); break;
			case WaveShape::Quintic: text += "quintic " + floatToString(part._voice._waveShapeParameter); break;
			case WaveShape::Cosine: text += "cosine"; break;
			}
		}
		std::for_each(impl._parts.cbegin(), impl._parts.cend(), [&floatToString, &text, partIndex = 1](const Part& part) mutable {
			std::for_each(part._tracks.cbegin(), part._tracks.cend(), [&floatToString, &text, partIndex, trackIndex = 1](const Track& track) mutable {
				text += "\n\n@track " + std::to_string(partIndex) + ' ' + std::to_string(trackIndex);
				text += "\npolyphony ";
				switch (track._properties._polyphony)
				{
				case Polyphony::Chord: text += "chord"; break;
				case Polyphony::Full: text += "full"; break;
				}
				text += "\nstereo_angle " + std::to_string(track._properties._sourceOffset);
				text += "\nstereo_angular_size " + std::to_string(track._properties._sourceWidth);
				text += "\nstereo_delay " + floatToString(track._properties._headDelay);
				text += "\nstereo_distance " + floatToString(track._properties._sourceDistance);
				text += "\nweight " + std::to_string(track._properties._weight);
				++trackIndex;
			});
			++partIndex;
		});
		text += "\n\n@sequences";
		for (const auto& part : impl._parts)
		{
			const auto partIndex = static_cast<size_t>(&part - impl._parts.data() + 1);
			for (const auto& track : part._tracks)
			{
				const auto trackIndex = static_cast<size_t>(&track - part._tracks.data() + 1);
				for (const auto& sequence : track._sequences)
				{
					const auto sequenceIndex = static_cast<size_t>(&sequence - track._sequences.data() + 1);
					text += '\n' + std::to_string(partIndex) + ' ' + std::to_string(trackIndex) + ' ' + std::to_string(sequenceIndex);
					if (!sequence.empty())
						text += ' ';
					for (const auto& sound : sequence)
					{
						text.append(sound._delay, ',');
						const auto note = static_cast<unsigned>(sound._note);
						switch (note % kNotesPerOctave)
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
						text += std::to_string(note / kNotesPerOctave);
					}
				}
			}
		}
		text += "\n\n@fragments";
		std::for_each(impl._parts.cbegin(), impl._parts.cend(), [&text, partIndex = 1](const Part& part) mutable {
			std::for_each(part._tracks.cbegin(), part._tracks.cend(), [&text, partIndex, trackIndex = 1](const Track& track) mutable {
				text += '\n' + std::to_string(partIndex) + ' ' + std::to_string(trackIndex);
				for (const auto& fragment : track._fragments)
					text += ' ' + std::to_string(fragment._delay) + ' ' + std::to_string(fragment._sequence + 1);
				++trackIndex;
			});
			++partIndex;
		});
		text += '\n';

		std::vector<std::byte> buffer;
		buffer.reserve(text.size());
		std::for_each(std::next(text.begin()), text.end(), [&buffer](const auto c) { buffer.emplace_back(static_cast<std::byte>(c)); });
		return buffer;
	}
}
