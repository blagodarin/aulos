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

#include <QPainter>

namespace
{
	constexpr auto kNoteHeight = 20.;
	constexpr auto kWhiteKeyWidth = 3 * kNoteHeight;
	constexpr auto kBlackKeyWidth = 2 * kNoteHeight;
}

KeyboardItem::KeyboardItem(QGraphicsItem* parent)
	: QGraphicsObject{ parent }
{
}

QRectF KeyboardItem::boundingRect() const
{
	return { 0.0, -kWhiteKeyWidth, kWhiteKeyWidth, 12 * kNoteHeight };
}

void KeyboardItem::paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*)
{
	const auto octaveRect = boundingRect();
	const auto topLeft = [&octaveRect](qreal y) {
		return octaveRect.topLeft() + QPointF{ 0.0, kNoteHeight * y };
	};
	const auto keySize = [&octaveRect](qreal width, qreal height = 1.0) {
		return QSizeF{ width, kNoteHeight * height };
	};
	painter->setPen(Qt::lightGray);
	painter->setBrush(Qt::white);
	painter->drawRect(QRectF{ topLeft(0.0), keySize(kWhiteKeyWidth, 1.5) });  // B
	painter->drawRect(QRectF{ topLeft(1.5), keySize(kWhiteKeyWidth, 2.0) });  // A
	painter->drawRect(QRectF{ topLeft(3.5), keySize(kWhiteKeyWidth, 2.0) });  // G
	painter->drawRect(QRectF{ topLeft(5.5), keySize(kWhiteKeyWidth, 1.5) });  // F
	painter->drawRect(QRectF{ topLeft(7.0), keySize(kWhiteKeyWidth, 1.5) });  // E
	painter->drawRect(QRectF{ topLeft(8.5), keySize(kWhiteKeyWidth, 2.0) });  // D
	painter->drawRect(QRectF{ topLeft(10.5), keySize(kWhiteKeyWidth, 1.5) }); // C
	painter->setPen(Qt::darkGray);
	painter->setBrush(Qt::black);
	painter->drawRect(QRectF{ topLeft(1.0), keySize(kBlackKeyWidth) });  // A#
	painter->drawRect(QRectF{ topLeft(3.0), keySize(kBlackKeyWidth) });  // G#
	painter->drawRect(QRectF{ topLeft(5.0), keySize(kBlackKeyWidth) });  // E#
	painter->drawRect(QRectF{ topLeft(8.0), keySize(kBlackKeyWidth) });  // D#
	painter->drawRect(QRectF{ topLeft(10.0), keySize(kBlackKeyWidth) }); // C#
}
