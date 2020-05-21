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
		std::vector<std::shared_ptr<PartData>> _parts;
		std::string _title;
		std::string _author;

		CompositionData() = default;
		CompositionData(const Composition&);

		[[nodiscard]] std::unique_ptr<Composition> pack() const;
	};

	std::vector<std::byte> serialize(const Composition&);
}
