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

#include <QPainter>
#include <QStyleOptionGraphicsItem>

PianorollItem::PianorollItem(QGraphicsItem* parent)
	: QGraphicsObject{ parent }
{
	setFlag(QGraphicsItem::ItemUsesExtendedStyleOption);
}

QRectF PianorollItem::boundingRect() const
{
	return { {}, QSizeF{ _stepCount * kStepWidth, 120 * kNoteHeight } };
}

void PianorollItem::setStepCount(size_t count)
{
	prepareGeometryChange();
	_stepCount = count;
}

void PianorollItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget*)
{
	if (!_stepCount)
		return;
	painter->setPen(Qt::transparent);
	const auto firstNote = static_cast<size_t>(std::floor(option->exposedRect.top() / kNoteHeight));
	const auto firstStep = static_cast<size_t>(std::floor(option->exposedRect.left() / kStepWidth));
	for (auto note = firstNote; note < 120; ++note)
	{
		QRectF rect{ { firstStep * kStepWidth, note * kNoteHeight }, QSizeF{ kStepWidth, kNoteHeight } };
		const auto& colors = kTrackColors[note % kTrackColors.size()]._colors;
		for (auto step = firstStep; step < _stepCount; ++step)
		{
			painter->setBrush(colors[step % colors.size()]);
			painter->drawRect(rect);
			if (rect.right() > option->exposedRect.right())
				break;
			rect.moveLeft(rect.right());
		}
		if (rect.bottom() > option->exposedRect.bottom())
			break;
	}
}
