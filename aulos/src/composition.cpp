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

#include "composition.hpp"

namespace aulos
{
	CompositionImpl::CompositionImpl()
	{
		_notes.emplace_back(Note::Silence);
		_notes.emplace_back(Note::E5);
		_notes.emplace_back(Note::Eb5);
		_notes.emplace_back(Note::E5);
		_notes.emplace_back(Note::Eb5);
		_notes.emplace_back(Note::E5);
		_notes.emplace_back(Note::B4);
		_notes.emplace_back(Note::D5);
		_notes.emplace_back(Note::C5);
		_notes.emplace_back(Note::A4, 3);
		_notes.emplace_back(Note::C4);
		_notes.emplace_back(Note::E4);
		_notes.emplace_back(Note::A4);
		_notes.emplace_back(Note::B4, 3);
		_notes.emplace_back(Note::E4);
		_notes.emplace_back(Note::A4);
		_notes.emplace_back(Note::B4);
		_notes.emplace_back(Note::C5, 3);
		_notes.emplace_back(Note::E4);
		_notes.emplace_back(Note::E5);
		_notes.emplace_back(Note::Eb5);
		_notes.emplace_back(Note::E5);
		_notes.emplace_back(Note::Eb5);
		_notes.emplace_back(Note::E5);
		_notes.emplace_back(Note::B4);
		_notes.emplace_back(Note::D5);
		_notes.emplace_back(Note::C5);
		_notes.emplace_back(Note::A4, 3);
		_notes.emplace_back(Note::C4);
		_notes.emplace_back(Note::E4);
		_notes.emplace_back(Note::A4);
		_notes.emplace_back(Note::B4, 3);
		_notes.emplace_back(Note::E4);
		_notes.emplace_back(Note::C5);
		_notes.emplace_back(Note::B4);
		_notes.emplace_back(Note::A4, 4);
	}

	std::unique_ptr<Composition> Composition::create()
	{
		return std::make_unique<CompositionImpl>();
	}
}
