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

#include "keyboard_item.hpp"

#include <array>

#include <QPainter>

namespace
{
	constexpr auto kNoteHeight = 20.0;
	constexpr auto kWhiteKeyWidth = 3 * kNoteHeight;
	constexpr auto kBlackKeyWidth = 2 * kNoteHeight;

	struct KeyInfo
	{
		QString _name;
		qreal _y;
		qreal _width;
		qreal _height;
		QColor _borderColor;
		QColor _textColor;
	};

	const QColor kWhiteKeyBorder{ "#aaa" };
	const QColor kWhiteKeyText{ "#999" };
	const QColor kBlackKeyBorder{ "#555" };
	const QColor kBlackKeyText{ "#999" };

	const std::array<KeyInfo, 12> kKeyInfo{
		KeyInfo{ QStringLiteral("B%1"), 0.0, kWhiteKeyWidth, 1.5, kWhiteKeyBorder, kWhiteKeyText },
		KeyInfo{ QStringLiteral("A#%1"), 1.0, kBlackKeyWidth, 1.0, kBlackKeyBorder, kBlackKeyText },
		KeyInfo{ QStringLiteral("A%1"), 1.5, kWhiteKeyWidth, 2.0, kWhiteKeyBorder, kWhiteKeyText },
		KeyInfo{ QStringLiteral("G#%1"), 3.0, kBlackKeyWidth, 1.0, kBlackKeyBorder, kBlackKeyText },
		KeyInfo{ QStringLiteral("G%1"), 3.5, kWhiteKeyWidth, 2.0, kWhiteKeyBorder, kWhiteKeyText },
		KeyInfo{ QStringLiteral("F#%1"), 5.0, kBlackKeyWidth, 1.0, kBlackKeyBorder, kBlackKeyText },
		KeyInfo{ QStringLiteral("F%1"), 5.5, kWhiteKeyWidth, 1.5, kWhiteKeyBorder, kWhiteKeyText },
		KeyInfo{ QStringLiteral("E%1"), 7.0, kWhiteKeyWidth, 1.5, kWhiteKeyBorder, kWhiteKeyText },
		KeyInfo{ QStringLiteral("D#%1"), 8.0, kBlackKeyWidth, 1.0, kBlackKeyBorder, kBlackKeyText },
		KeyInfo{ QStringLiteral("D%1"), 8.5, kWhiteKeyWidth, 2.0, kWhiteKeyBorder, kWhiteKeyText },
		KeyInfo{ QStringLiteral("C#%1"), 10.0, kBlackKeyWidth, 1.0, kBlackKeyBorder, kBlackKeyText },
		KeyInfo{ QStringLiteral("C%1"), 10.5, kWhiteKeyWidth, 1.5, kWhiteKeyBorder, kWhiteKeyText },
	};
}

KeyboardItem::KeyboardItem(QGraphicsItem* parent)
	: QGraphicsObject{ parent }
{
}

QRectF KeyboardItem::boundingRect() const
{
	return { 0.0, -kWhiteKeyWidth, kWhiteKeyWidth, 120 * kNoteHeight };
}

void KeyboardItem::paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*)
{
	const auto drawKey = [painter](const QPointF& topLeft, int octave, size_t note) {
		const auto& key = kKeyInfo[note];
		painter->setPen(key._borderColor);
		painter->drawRect(QRectF{ { topLeft.x(), topLeft.y() + kNoteHeight * key._y }, QSizeF{ key._width, kNoteHeight * key._height } });
		painter->setPen(key._textColor);
		painter->drawText(QRectF{ { topLeft.x(), topLeft.y() + kNoteHeight * note }, QSizeF{ key._width - kNoteHeight * 0.125, kNoteHeight } }, Qt::AlignVCenter | Qt::AlignRight, key._name.arg(octave));
	};

	auto octaveRect = boundingRect();
	octaveRect.setHeight(12 * kNoteHeight);
	for (int octave = 9; octave >= 0; --octave)
	{
		painter->setBrush(Qt::white);
		drawKey(octaveRect.topLeft(), octave, 0);
		drawKey(octaveRect.topLeft(), octave, 2);
		drawKey(octaveRect.topLeft(), octave, 4);
		drawKey(octaveRect.topLeft(), octave, 6);
		drawKey(octaveRect.topLeft(), octave, 7);
		drawKey(octaveRect.topLeft(), octave, 9);
		drawKey(octaveRect.topLeft(), octave, 11);
		painter->setBrush(Qt::black);
		drawKey(octaveRect.topLeft(), octave, 1);
		drawKey(octaveRect.topLeft(), octave, 3);
		drawKey(octaveRect.topLeft(), octave, 5);
		drawKey(octaveRect.topLeft(), octave, 8);
		drawKey(octaveRect.topLeft(), octave, 10);
		octaveRect.moveTop(octaveRect.bottom());
	}
}
