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

#include "composition_scene.hpp"

#include <array>

#include <QGraphicsSceneContextMenuEvent>
#include <QMenu>
#include <QPainter>

namespace
{
	const std::array<QColor, 2> kTrackColors{ "#999", "#888" };
}

TrackItem::TrackItem(size_t trackIndex, size_t length, QGraphicsItem* parent)
	: QGraphicsObject{ parent }
	, _trackIndex{ trackIndex }
	, _length{ length }
	, _rect{ 0.0, _trackIndex * kScaleY, _length * kScaleX, kScaleY }
{
}

void TrackItem::paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*)
{
	painter->setBrush(kTrackColors[_trackIndex % kTrackColors.size()]);
	painter->setPen(Qt::transparent);
	painter->drawRect(_rect);
}

void TrackItem::setTrackLength(size_t length)
{
	if (_length == length)
		return;
	prepareGeometryChange();
	_length = length;
	_rect.setWidth(_length * kScaleX);
}

void TrackItem::contextMenuEvent(QGraphicsSceneContextMenuEvent* e)
{
	QMenu menu;
	menu.addAction(tr("Insert..."));
	if (menu.exec(e->screenPos()))
		emit insertRequested(_trackIndex, static_cast<size_t>(std::ceil(e->pos().x() - _rect.left()) / kScaleX));
}
