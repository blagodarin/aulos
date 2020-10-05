// This file is part of the Aulos toolkit.
// Copyright (C) Sergei Blagodarin.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "../button_item.hpp"

#include <aulos/data.hpp>

#include <array>

class KeyItem final : public ButtonItem
{
	Q_OBJECT

public:
	explicit KeyItem(aulos::Note, QGraphicsItem* parent = nullptr);

	QRectF boundingRect() const override;
	void paint(QPainter*, const QStyleOptionGraphicsItem*, QWidget*) override;

private:
	struct NoteInfo;
	struct StyleInfo;

	static const std::array<NoteInfo, aulos::kNotesPerOctave> kNoteInfo;
	static const std::array<StyleInfo, 2> kStyleInfo;

	const size_t _octave;
	const NoteInfo& _noteInfo;
	const StyleInfo& _styleInfo;
};
