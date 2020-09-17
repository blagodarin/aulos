// This file is part of the Aulos toolkit.
// Copyright (C) Sergei Blagodarin.
// SPDX-License-Identifier: Apache-2.0

#include "track_item.hpp"

#include "../theme.hpp"

#include <cmath>

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
