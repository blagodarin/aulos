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

			const auto partIndex = static_cast<size_t>(&part - impl._parts.data() + 1);
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
		for (const auto& part : impl._parts)
		{
			const auto partIndex = static_cast<size_t>(&part - impl._parts.data() + 1);
			for (const auto& track : part._tracks)
			{
				const auto trackIndex = static_cast<size_t>(&track - part._tracks.data() + 1);
				text += '\n' + std::to_string(partIndex) + ' ' + std::to_string(trackIndex) + ' ' + std::to_string(track._weight);
			}
		}
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
		for (const auto& part : impl._parts)
		{
			const auto partIndex = static_cast<size_t>(&part - impl._parts.data() + 1);
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
}
