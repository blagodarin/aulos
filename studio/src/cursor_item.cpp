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

#include "cursor_item.hpp"

#include "colors.hpp"
#include "utils.hpp"

#include <QPainter>

CursorItem::CursorItem(QGraphicsItem* parent)
	: QGraphicsItem{ parent }
{
}

QRectF CursorItem::boundingRect() const
{
	return { 0, 0, 1, kTimelineHeight + kTrackHeight * _trackCount };
}

void CursorItem::paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*)
{
	painter->setPen(Qt::transparent);
	painter->setBrush(kCursorColor);
	painter->drawRect(boundingRect());
}

void CursorItem::setTrackCount(size_t count)
{
	prepareGeometryChange();
	_trackCount = count;
}
