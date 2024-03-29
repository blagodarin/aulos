// This file is part of the Aulos toolkit.
// Copyright (C) Sergei Blagodarin.
// SPDX-License-Identifier: Apache-2.0

#include "sound_item.hpp"

#include "../theme.hpp"

#include <QGraphicsSceneEvent>
#include <QPainter>

SoundItem::SoundItem(size_t offset, seir::synth::Note note, size_t sustain, QGraphicsItem* parent)
	: QGraphicsObject{ parent }
	, _offset{ offset }
	, _note{ note }
	, _sustain{ sustain }
{
	setPos(_offset * kNoteWidth, (seir::synth::kNoteCount - 1 - static_cast<size_t>(_note)) * kNoteHeight);
}

void SoundItem::setSustain(size_t sustain)
{
	prepareGeometryChange();
	_sustain = sustain;
}

QRectF SoundItem::boundingRect() const
{
	return { 0, 0, kNoteWidth * (1 + _sustain), kNoteHeight };
}

void SoundItem::mousePressEvent(QGraphicsSceneMouseEvent* e)
{
	if (const auto button = e->button(); button == Qt::LeftButton)
	{
		e->accept();
		if (e->modifiers() & Qt::ShiftModifier)
			emit increaseSustainRequested();
		else
			emit playRequested();
	}
	else if (button == Qt::RightButton)
	{
		e->accept();
		if (e->modifiers() & Qt::ShiftModifier)
			emit decreaseSustainRequested();
		else
			emit removeRequested();
	}
}

void SoundItem::paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*)
{
	painter->setPen(kSoundBorderColor);
	painter->setBrush(kSoundBackgroundColor);
	painter->drawRect(boundingRect());
}
