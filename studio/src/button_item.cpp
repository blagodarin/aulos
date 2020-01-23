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

#include "button_item.hpp"

#include <QGraphicsSceneEvent>

ButtonItem::ButtonItem(QGraphicsItem* parent)
	: QGraphicsObject{ parent }
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
	}
}

void ButtonItem::mouseReleaseEvent(QGraphicsSceneMouseEvent* e)
{
	_pressed = false;
	update();
	if (boundingRect().contains(e->lastPos()))
		emit clicked();
}
