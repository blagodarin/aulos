// This file is part of the Aulos toolkit.
// Copyright (C) Sergei Blagodarin.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <aulos/voice.hpp>

#include <map>
#include <string>

namespace aulos
{
	struct SequenceData
	{
		std::vector<Sound> _sounds;
	};

	struct TrackData
	{
		unsigned _weight = 0;
		std::vector<std::shared_ptr<SequenceData>> _sequences;
		std::map<size_t, std::shared_ptr<SequenceData>> _fragments;

		explicit TrackData(unsigned weight) noexcept
			: _weight{ weight } {}
	};

	struct PartData
	{
		std::shared_ptr<VoiceData> _voice;
		std::string _voiceName;
		std::vector<std::shared_ptr<TrackData>> _tracks;

		explicit PartData(const std::shared_ptr<VoiceData>& voice) noexcept
			: _voice{ voice } {}
	};

	// Contains composition data in an editable format.
	struct CompositionData
	{
		unsigned _speed = kMinSpeed;
		unsigned _loopOffset = 0;
		unsigned _loopLength = 0;
		std::vector<std::shared_ptr<PartData>> _parts;
		std::string _title;
		std::string _author;

		CompositionData() = default;
		CompositionData(const Composition&);
		CompositionData(const std::shared_ptr<VoiceData>&, Note);

		[[nodiscard]] std::unique_ptr<Composition> pack() const;
	};

	std::vector<std::byte> serialize(const Composition&);
}
