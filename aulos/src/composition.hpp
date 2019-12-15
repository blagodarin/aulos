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

#pragma once

#include <aulos.hpp>

namespace aulos
{
	struct CompositionImpl final : public Composition
	{
		float _speed;
		std::vector<Voice> _voices;
		std::vector<Track> _tracks;
		std::vector<std::vector<Sound>> _sequences;
		std::vector<Fragment> _fragments;

		CompositionImpl();

		void load(const char* source);

		Fragment fragment(size_t index) const noexcept override { return index < _fragments.size() ? _fragments[index] : Fragment{}; }
		size_t fragmentCount() const noexcept override { return _fragments.size(); }
		float speed() const noexcept override { return _speed; }
		Sequence sequence(size_t index) const noexcept override;
		size_t sequenceCount() const noexcept override { return _sequences.size(); }
		Track track(size_t index) const noexcept override { return index < _tracks.size() ? _tracks[index] : Track{}; }
		size_t trackCount() const noexcept override { return _tracks.size(); }
	};
}
