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

#include <aulos/data.hpp>

namespace aulos
{
	struct Fragment
	{
		size_t _delay = 0;
		size_t _sequence = 0;

		constexpr Fragment(size_t delay, size_t sequence) noexcept
			: _delay{ delay }, _sequence{ sequence } {}
	};

	struct Track
	{
		size_t _voice = 0;
		unsigned _weight = 0;
		std::vector<std::vector<Sound>> _sequences;
		std::vector<Fragment> _fragments;

		Track(size_t voice, unsigned weight) noexcept
			: _voice{ voice }, _weight{ weight } {}
	};

	struct CompositionImpl final : public Composition
	{
		float _speed = kMinSpeed;
		std::vector<Voice> _voices;
		std::vector<Track> _tracks;

		void load(const char* source);
	};
}
