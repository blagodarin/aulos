// This file is part of the Aulos toolkit.
// Copyright (C) Sergei Blagodarin.
// SPDX-License-Identifier: Apache-2.0

#include "pianoroll_item.hpp"

#include "../theme.hpp"

#include <cassert>
#include <cmath>

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
	return { 0, 0, _stepCount * kNoteWidth, 120 * kNoteHeight };
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
		assert(row < 120);
		const auto offset = static_cast<size_t>(std::floor(pos.x() / kNoteWidth));
		emit newSoundRequested(offset, static_cast<aulos::Note>(119 - row));
	}
}

void PianorollItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget*)
{
	if (!_stepCount)
		return;
	const auto firstRow = static_cast<size_t>(std::floor(option->exposedRect.top() / kNoteHeight));
	const auto lastRow = static_cast<size_t>(std::ceil(option->exposedRect.bottom() / kNoteHeight));
	const auto firstStep = static_cast<size_t>(std::floor(option->exposedRect.left() / kNoteWidth));
	const auto lastStep = static_cast<size_t>(std::ceil(option->exposedRect.right() / kNoteWidth));
	for (auto row = firstRow; row < lastRow; ++row)
	{
		const auto rowTop = row * kNoteHeight;
		painter->setPen(Qt::transparent);
		painter->setBrush(kPianorollBackgroundColor[::rowToColorIndex(row)]);
		painter->drawRect({ { option->exposedRect.left(), rowTop }, QSizeF{ option->exposedRect.width(), kNoteHeight } });
		painter->setPen((row % 12) ? kPianorollFineGridColor : kPianorollCoarseGridColor);
		painter->drawLine(QPointF{ option->exposedRect.left(), rowTop }, QPointF{ option->exposedRect.right(), rowTop });
	}
	for (auto step = firstStep; step < lastStep; ++step)
	{
		const auto stepLeft = step * kNoteWidth;
		painter->setPen((step % kPianorollStride) ? kPianorollFineGridColor : kPianorollCoarseGridColor);
		painter->drawLine(QPointF{ stepLeft, option->exposedRect.top() }, QPointF{ stepLeft, option->exposedRect.bottom() });
	}
}
