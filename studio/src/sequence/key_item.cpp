// This file is part of the Aulos toolkit.
// Copyright (C) Sergei Blagodarin.
// SPDX-License-Identifier: Apache-2.0

#include "key_item.hpp"

#include "../theme.hpp"

#include <QPainter>

namespace
{
	enum class KeyStyle : size_t
	{
		White,
		Black,
	};
}

struct KeyItem::StyleInfo
{
	struct Colors
	{
		QColor _normal;
		QColor _hovered;
		QColor _pressed;

		const QColor& operator()(bool hovered, bool pressed) const noexcept
		{
			return pressed ? _pressed : (hovered ? _hovered : _normal);
		}
	};

	qreal _width;
	Colors _backgroundColors;
	Colors _borderColors;
	Colors _textColors;
	qreal _z;
};

const std::array<KeyItem::StyleInfo, 2> KeyItem::kStyleInfo{
	StyleInfo{ kWhiteKeyWidth, { "#fff", "#fdd", "#fcc" }, { "#aaa", "#aaa", "#aaa" }, { "#999", "#944", "#900" }, 0.5 },
	StyleInfo{ kBlackKeyWidth, { "#000", "#200", "#300" }, { "#555", "#500", "#500" }, { "#999", "#f99", "#f99" }, 1.0 },
};

struct KeyItem::NoteInfo
{
	QString _name;
	qreal _y;
	qreal _height;
	qreal _textOffset;
	KeyStyle _style;
};

const std::array<KeyItem::NoteInfo, aulos::kNotesPerOctave> KeyItem::kNoteInfo{
	NoteInfo{ QStringLiteral("C%1"), 10.5, 1.5, 0.5, KeyStyle::White },
	NoteInfo{ QStringLiteral("C#%1"), 10.0, 1.0, 0.0, KeyStyle::Black },
	NoteInfo{ QStringLiteral("D%1"), 8.5, 2.0, 0.5, KeyStyle::White },
	NoteInfo{ QStringLiteral("D#%1"), 8.0, 1.0, 0.0, KeyStyle::Black },
	NoteInfo{ QStringLiteral("E%1"), 7.0, 1.5, 0.0, KeyStyle::White },
	NoteInfo{ QStringLiteral("F%1"), 5.5, 1.5, 0.5, KeyStyle::White },
	NoteInfo{ QStringLiteral("F#%1"), 5.0, 1.0, 0.0, KeyStyle::Black },
	NoteInfo{ QStringLiteral("G%1"), 3.5, 2.0, 0.5, KeyStyle::White },
	NoteInfo{ QStringLiteral("G#%1"), 3.0, 1.0, 0.0, KeyStyle::Black },
	NoteInfo{ QStringLiteral("A%1"), 1.5, 2.0, 0.5, KeyStyle::White },
	NoteInfo{ QStringLiteral("A#%1"), 1.0, 1.0, 0.0, KeyStyle::Black },
	NoteInfo{ QStringLiteral("B%1"), 0.0, 1.5, 0.0, KeyStyle::White },
};

KeyItem::KeyItem(aulos::Note note, QGraphicsItem* parent)
	: ButtonItem{ Mode::Press, parent }
	, _octave{ static_cast<size_t>(note) / kNoteInfo.size() }
	, _noteInfo{ kNoteInfo[static_cast<size_t>(note) % kNoteInfo.size()] }
	, _styleInfo{ kStyleInfo[static_cast<size_t>(_noteInfo._style)] }
{
	setPos(0, ((aulos::kOctaveCount - 1 - _octave) * kNoteInfo.size() + _noteInfo._y) * kNoteHeight);
	setZValue(_styleInfo._z);
}

QRectF KeyItem::boundingRect() const
{
	return { 0, 0, _styleInfo._width, _noteInfo._height * kNoteHeight };
}

void KeyItem::paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*)
{
	const auto rect = boundingRect();
	painter->setBrush(_styleInfo._backgroundColors(isHovered(), isPressed()));
	painter->setPen(Qt::transparent);
	painter->drawRect(rect);
	painter->setPen(_styleInfo._borderColors(isHovered(), isPressed()));
	painter->drawLine(rect.topLeft(), rect.topRight());
	painter->drawLine(rect.topRight(), rect.bottomRight());
	painter->drawLine(rect.bottomRight(), rect.bottomLeft());
	painter->setPen(_styleInfo._textColors(isHovered(), isPressed()));
	painter->drawText(QRectF{ { 0, _noteInfo._textOffset * kNoteHeight }, QSizeF{ _styleInfo._width - kNoteHeight * 0.125, kNoteHeight } }, Qt::AlignVCenter | Qt::AlignRight, _noteInfo._name.arg(_octave));
}
