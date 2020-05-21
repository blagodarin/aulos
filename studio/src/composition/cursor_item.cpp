// This file is part of the Aulos toolkit.
// Copyright (C) Sergei Blagodarin.
// SPDX-License-Identifier: Apache-2.0

#include "cursor_item.hpp"

#include "../theme.hpp"

#include <QPainter>

CursorItem::CursorItem(QGraphicsItem* parent)
	: QGraphicsItem{ parent }
{
}

QRectF CursorItem::boundingRect() const
{
	return { 0, 0, 2, kTimelineHeight + kTrackHeight * _trackCount };
}

void CursorItem::paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*)
{
	painter->setPen(kCursorColors._pen);
	painter->setBrush(kCursorColors._brush);
	painter->drawRect(boundingRect());
}

void CursorItem::setTrackCount(size_t count)
{
	prepareGeometryChange();
	_trackCount = count;
}
