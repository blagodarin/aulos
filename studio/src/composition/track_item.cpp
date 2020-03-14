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

#include "track_item.hpp"

#include "../colors.hpp"
#include "../utils.hpp"

#include <QGraphicsSceneContextMenuEvent>
#include <QMenu>
#include <QPainter>
#include <QStyleOptionGraphicsItem>

TrackItem::TrackItem(const void* id, QGraphicsItem* parent)
	: QGraphicsObject{ parent }
	, _trackId{ id }
{
	setFlag(QGraphicsItem::ItemUsesExtendedStyleOption);
}

QRectF TrackItem::boundingRect() const
{
	return { 0, 0, _length * kStepWidth, kTrackHeight };
}

void TrackItem::setFirstTrack(bool first)
{
	_first = first;
	update();
}

void TrackItem::setTrackIndex(size_t index)
{
	_index = index;
	update();
}

void TrackItem::setTrackLength(size_t length)
{
	prepareGeometryChange();
	_length = length;
}

void TrackItem::contextMenuEvent(QGraphicsSceneContextMenuEvent* e)
{
	emit trackMenuRequested(_trackId, static_cast<size_t>(std::ceil(e->pos().x()) / kStepWidth), e->screenPos());
}

void TrackItem::mouseDoubleClickEvent(QGraphicsSceneMouseEvent*)
{
	emit trackActionRequested(_trackId);
}

void TrackItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget*)
{
	if (!_length)
		return;
	const auto& colors = kTrackColors[_index % kTrackColors.size()]._colors;
	auto step = static_cast<size_t>(std::floor(option->exposedRect.left() / kStepWidth));
	QRectF rect{ { step * kStepWidth, 0 }, QSizeF{ kStepWidth, kTrackHeight } };
	painter->setPen(Qt::transparent);
	while (step < _length)
	{
		painter->setBrush(colors[step % colors.size()]);
		painter->drawRect(rect);
		if (rect.right() > option->exposedRect.right())
			break;
		rect.moveLeft(rect.right());
		++step;
	}
	if (_first)
	{
		painter->setPen(kPartBorderColor);
		painter->drawLine(boundingRect().topLeft(), boundingRect().topRight() - QPointF{ 1, 0 });
	}
}
