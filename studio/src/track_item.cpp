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

#include <aulos/data.hpp>

#include <array>

#include <QGraphicsSceneContextMenuEvent>
#include <QMenu>
#include <QPainter>

namespace
{
	const std::array<QColor, 2> kTrackColors{ "#999", "#888" };
}

TrackItem::TrackItem(const std::shared_ptr<const aulos::CompositionData>& composition, size_t trackIndex, size_t length, QGraphicsItem* parent)
	: QGraphicsObject{ parent }
	, _composition{ composition }
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
	const auto insertSubmenu = menu.addMenu(tr("Insert"));
	const auto sequenceCount = _composition->_tracks[_trackIndex]._sequences.size();
	for (size_t sequenceIndex = 0; sequenceIndex < sequenceCount; ++sequenceIndex)
		insertSubmenu->addAction(tr("Sequence %1").arg(sequenceIndex + 1))->setData(sequenceIndex);
	if (sequenceCount > 0)
		insertSubmenu->addSeparator();
	const auto newSequenceAction = insertSubmenu->addAction(tr("New sequence..."));
	const auto action = menu.exec(e->screenPos());
	if (!action)
		return;
	const auto offset = static_cast<size_t>(std::ceil(e->pos().x() - _rect.left()) / kScaleX);
	if (action == newSequenceAction)
		emit newSequenceRequested(_trackIndex, offset);
	else
		emit insertRequested(_trackIndex, offset, static_cast<size_t>(action->data().toInt()));
}
