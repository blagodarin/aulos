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
		switch (seir::synth::kNotesPerOctave - row % seir::synth::kNotesPerOctave)
		{
		case 1:
		case 3:
		case 5:
		case 6:
		case 8:
		case 10:
		case 12:
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
	return { 0, 0, _stepCount * kNoteWidth, seir::synth::kNoteCount * kNoteHeight };
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
		assert(row < seir::synth::kNoteCount);
		const auto offset = static_cast<size_t>(std::floor(pos.x() / kNoteWidth));
		emit newSoundRequested(offset, static_cast<seir::synth::Note>(seir::synth::kNoteCount - 1 - row));
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
		painter->setPen((row % seir::synth::kNotesPerOctave) ? kPianorollFineGridColor : kPianorollCoarseGridColor);
		painter->drawLine(QPointF{ option->exposedRect.left(), rowTop }, QPointF{ option->exposedRect.right(), rowTop });
	}
	for (auto step = firstStep; step < lastStep; ++step)
	{
		const auto stepLeft = step * kNoteWidth;
		painter->setPen((step % kPianorollStride) ? kPianorollFineGridColor : kPianorollCoarseGridColor);
		painter->drawLine(QPointF{ stepLeft, option->exposedRect.top() }, QPointF{ stepLeft, option->exposedRect.bottom() });
	}
}
