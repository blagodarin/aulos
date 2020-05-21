// This file is part of the Aulos toolkit.
// Copyright (C) Sergei Blagodarin.
// SPDX-License-Identifier: Apache-2.0

#include "button_item.hpp"

#include <QGraphicsSceneEvent>

ButtonItem::ButtonItem(Mode mode, QGraphicsItem* parent)
	: QGraphicsObject{ parent }
	, _mode{ mode }
{
	setAcceptHoverEvents(true);
}

void ButtonItem::hoverEnterEvent(QGraphicsSceneHoverEvent* e)
{
	e->accept();
	_hovered = true;
	update();
}

void ButtonItem::hoverLeaveEvent(QGraphicsSceneHoverEvent* e)
{
	e->accept();
	_hovered = false;
	update();
}

void ButtonItem::mousePressEvent(QGraphicsSceneMouseEvent* e)
{
	if (e->setAccepted(e->button() == Qt::LeftButton); e->isAccepted())
	{
		_pressed = true;
		update();
		if (_mode == Mode::Press)
			emit activated();
	}
}

void ButtonItem::mouseReleaseEvent(QGraphicsSceneMouseEvent* e)
{
	_pressed = false;
	update();
	if (_mode == Mode::Click && boundingRect().contains(e->lastPos()))
		emit activated();
}
