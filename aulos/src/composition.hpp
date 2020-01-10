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

#include <aulos.hpp>

namespace aulos
{
	struct TrackData
	{
		Track _track;
		std::vector<std::vector<Sound>> _sequences;
		std::vector<Fragment> _fragments;

		TrackData(size_t voice, unsigned weight) noexcept
			: _track{ voice, weight } {}
	};

	struct CompositionImpl final : public Composition
	{
		float _speed;
		std::vector<Voice> _voices;
		std::vector<TrackData> _tracks;

		CompositionImpl();

		void load(const char* source);

		Fragment fragment(size_t track, size_t index) const noexcept override;
		size_t fragmentCount(size_t track) const noexcept override { return track < _tracks.size() ? _tracks[track]._fragments.size() : 0; }
		float speed() const noexcept override { return _speed; }
		Sequence sequence(size_t track, size_t index) const noexcept override;
		size_t sequenceCount(size_t track) const noexcept override { return track < _tracks.size() ? _tracks[track]._sequences.size() : 0; }
		Track track(size_t index) const noexcept override { return index < _tracks.size() ? _tracks[index]._track : Track{}; }
		size_t trackCount() const noexcept override { return _tracks.size(); }
		Voice voice(size_t index) const noexcept override { return index < _voices.size() ? _voices[index] : Voice{}; }
		size_t voiceCount() const noexcept override { return _voices.size(); }
	};
}
