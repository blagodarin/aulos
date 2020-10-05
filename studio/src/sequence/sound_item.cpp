// This file is part of the Aulos toolkit.
// Copyright (C) Sergei Blagodarin.
// SPDX-License-Identifier: Apache-2.0

#include "sound_item.hpp"

#include "../theme.hpp"

#include <QGraphicsSceneEvent>
#include <QPainter>

SoundItem::SoundItem(size_t offset, aulos::Note note, QGraphicsItem* parent)
	: QGraphicsObject{ parent }
	, _offset{ offset }
{
	setNote(note);
}

void SoundItem::setNote(aulos::Note note)
{
	_note = note;
	setPos(_offset * kNoteWidth, (aulos::kNoteCount - 1 - static_cast<size_t>(_note)) * kNoteHeight);
}

QRectF SoundItem::boundingRect() const
{
	return { 0, 0, kNoteWidth, kNoteHeight };
}

void SoundItem::mousePressEvent(QGraphicsSceneMouseEvent* e)
{
	if (const auto button = e->button(); button == Qt::LeftButton)
	{
		e->accept();
		emit playRequested();
	}
	else if (button == Qt::RightButton)
	{
		e->accept();
		emit removeRequested();
	}
}

void SoundItem::paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*)
{
	painter->setPen(kSoundBorderColor);
	painter->setBrush(kSoundBackgroundColor);
	painter->drawRect(boundingRect());
}
