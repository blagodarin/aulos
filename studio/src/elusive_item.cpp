// This file is part of the Aulos toolkit.
// Copyright (C) Sergei Blagodarin.
// SPDX-License-Identifier: Apache-2.0

#include "elusive_item.hpp"

ElusiveItem::ElusiveItem(QGraphicsItem* parent)
	: QGraphicsObject{ parent }
{
}

void ElusiveItem::setHeight(qreal height)
{
	prepareGeometryChange();
	_height = height;
}

QRectF ElusiveItem::boundingRect() const
{
	return { 0, 0, 1, _height };
}

void ElusiveItem::paint(QPainter*, const QStyleOptionGraphicsItem*, QWidget*)
{
	emit elude();
}
