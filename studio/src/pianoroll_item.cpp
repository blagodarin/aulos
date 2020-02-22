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

#include "pianoroll_item.hpp"

#include "colors.hpp"
#include "utils.hpp"

#include <cassert>

#include <QGraphicsSceneEvent>
#include <QPainter>
#include <QStyleOptionGraphicsItem>

namespace
{
	constexpr size_t rowToColorIndex(size_t row)
	{
		switch (11 - row % 12)
		{
		case 0:
		case 2:
		case 4:
		case 5:
		case 7:
		case 9:
		case 11:
			return 0; // White.
		default:
			return 1; // Black.
		}
	}
}

PianorollItem::PianorollItem(QGraphicsItem* parent)
	: QGraphicsObject{ parent }
{
	setFlag(QGraphicsItem::ItemUsesExtendedStyleOption);
}

QRectF PianorollItem::boundingRect() const
{
	return { 0, 0, _stepCount * kStepWidth, 120 * kNoteHeight };
}

void PianorollItem::setStepCount(size_t count)
{
	prepareGeometryChange();
	_stepCount = count;
}

void PianorollItem::mousePressEvent(QGraphicsSceneMouseEvent* e)
{
	if (e->setAccepted(e->button() == Qt::LeftButton); e->isAccepted())
	{
		const auto pos = e->lastPos();
		const auto row = static_cast<size_t>(std::floor(pos.y() / kNoteHeight));
		assert(row >= 0 && row < 120);
		const auto offset = static_cast<size_t>(std::floor(pos.x() / kStepWidth));
		emit newSoundRequested(offset, static_cast<aulos::Note>(119 - row));
	}
}

void PianorollItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget*)
{
	if (!_stepCount)
		return;
	const auto firstRow = static_cast<size_t>(std::floor(option->exposedRect.top() / kNoteHeight));
	const auto lastRow = static_cast<size_t>(std::ceil(option->exposedRect.bottom() / kNoteHeight));
	const auto firstStep = static_cast<size_t>(std::floor(option->exposedRect.left() / kStepWidth));
	const auto lastStep = static_cast<size_t>(std::ceil(option->exposedRect.right() / kStepWidth));
	for (auto row = firstRow; row < lastRow; ++row)
	{
		const auto rowTop = row * kNoteHeight;
		const auto rowBottom = rowTop + kNoteHeight;
		painter->setPen(Qt::transparent);
		painter->setBrush(kPianorollBackgroundColor[::rowToColorIndex(row)]);
		painter->drawRect({ { option->exposedRect.left(), rowTop }, QSizeF{ option->exposedRect.width(), kNoteHeight } });
		painter->setPen(kPianorollGridColor);
		for (auto step = firstStep; step < lastStep; ++step)
		{
			const auto stepLeft = step * kStepWidth;
			painter->drawLine(QPointF{ stepLeft, rowTop }, QPointF{ stepLeft, rowBottom });
		}
		painter->setPen(row % 12 ? kPianorollGridColor : kPianorollOctaveBorderColor);
		painter->drawLine(QPointF{ option->exposedRect.left(), rowTop }, QPointF{ option->exposedRect.right(), rowTop });
	}
}
