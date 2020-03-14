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

#include "../button_item.hpp"

#include <aulos/data.hpp>

#include <array>

class KeyItem final : public ButtonItem
{
	Q_OBJECT

public:
	KeyItem(aulos::Note, QGraphicsItem* parent = nullptr);

	QRectF boundingRect() const override;
	void paint(QPainter*, const QStyleOptionGraphicsItem*, QWidget*) override;

private:
	struct NoteInfo;
	struct StyleInfo;

	static const std::array<NoteInfo, 12> kNoteInfo;
	static const std::array<StyleInfo, 2> kStyleInfo;

	const int _octave;
	const NoteInfo& _noteInfo;
	const StyleInfo& _styleInfo;
};
