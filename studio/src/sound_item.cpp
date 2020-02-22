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

#include "sound_item.hpp"

#include "colors.hpp"
#include "utils.hpp"

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
	setPos(_offset * kStepWidth, (119 - static_cast<size_t>(_note)) * kNoteHeight);
}

QRectF SoundItem::boundingRect() const
{
	return { 0, 0, kStepWidth, kNoteHeight };
}

void SoundItem::mousePressEvent(QGraphicsSceneMouseEvent* e)
{
	switch (e->button())
	{
	case Qt::LeftButton:
		e->accept();
		emit playRequested();
		break;
	case Qt::RightButton:
		e->accept();
		emit removeRequested();
		break;
	}
}

void SoundItem::paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*)
{
	painter->setPen(kSoundBorderColor);
	painter->setBrush(kSoundBackgroundColor);
	painter->drawRect(boundingRect());
}
