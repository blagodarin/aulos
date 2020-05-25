// This file is part of the Aulos toolkit.
// Copyright (C) Sergei Blagodarin.
// SPDX-License-Identifier: Apache-2.0

#include "loop_item.hpp"

#include "../theme.hpp"

#include <QGraphicsSceneEvent>
#include <QPainter>

LoopItem::LoopItem(QGraphicsItem* parent)
	: QGraphicsObject{ parent }
{
}

QRectF LoopItem::boundingRect() const
{
	return { 0, 0, kStepWidth * _loopLength, kLoopItemHeight };
}

void LoopItem::paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*)
{
	painter->setPen(kLoopItemColors._pen);
	painter->setBrush(kLoopItemColors._brush);
	painter->drawRoundedRect(boundingRect().adjusted(1, 0, -1, 0), kLoopItemHeight / 2, kLoopItemHeight / 2);
}

void LoopItem::setLoopLength(size_t length)
{
	prepareGeometryChange();
	_loopLength = length;
}

void LoopItem::contextMenuEvent(QGraphicsSceneContextMenuEvent* e)
{
	e->accept();
	emit menuRequested(e->screenPos());
}
