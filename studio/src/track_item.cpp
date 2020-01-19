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

#include "utils.hpp"

#include <aulos/data.hpp>

#include <array>

#include <QGraphicsSceneContextMenuEvent>
#include <QMenu>
#include <QPainter>
#include <QStyleOptionGraphicsItem>

namespace
{
	struct TrackColors
	{
		std::array<QColor, 2> _colors;
	};

	const std::array<TrackColors, 2> kTrackColors{
		TrackColors{ "#888", "#999" },
		TrackColors{ "#777", "#888" },
	};
}

TrackItem::TrackItem(const std::shared_ptr<const aulos::CompositionData>& composition, size_t trackIndex, QGraphicsItem* parent)
	: QGraphicsObject{ parent }
	, _composition{ composition }
	, _trackIndex{ trackIndex }
{
	setFlag(QGraphicsItem::ItemUsesExtendedStyleOption);
}

QRectF TrackItem::boundingRect() const
{
	return { {}, QSizeF{ _length * kStepWidth, kTrackHeight } };
}

void TrackItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget*)
{
	if (!_length)
		return;
	const auto& colors = kTrackColors[_trackIndex % kTrackColors.size()]._colors;
	auto step = static_cast<size_t>(std::floor(option->exposedRect.left() / kStepWidth));
	QRectF rect{ { step * kStepWidth, 0 }, QSizeF{ kStepWidth, kTrackHeight } };
	painter->setPen(Qt::transparent);
	while (step < _length - 1)
	{
		painter->setBrush(colors[step % colors.size()]);
		painter->drawRect(rect);
		if (rect.right() > option->exposedRect.right())
			return;
		rect.moveLeft(rect.right());
		++step;
	}
	rect.setRight(_length * kStepWidth);
	painter->setBrush(colors[step % colors.size()]);
	painter->drawRect(rect);
}

void TrackItem::setTrackLength(size_t length)
{
	if (_length == length)
		return;
	prepareGeometryChange();
	_length = length;
}

void TrackItem::contextMenuEvent(QGraphicsSceneContextMenuEvent* e)
{
	QMenu menu;
	const auto insertSubmenu = menu.addMenu(tr("Insert"));
	const auto sequenceCount = _composition->_tracks[_trackIndex]->_sequences.size();
	for (size_t sequenceIndex = 0; sequenceIndex < sequenceCount; ++sequenceIndex)
		insertSubmenu->addAction(::makeSequenceName(*_composition->_tracks[_trackIndex]->_sequences[sequenceIndex]))->setData(sequenceIndex);
	if (!_composition->_tracks[_trackIndex]->_sequences.empty())
		insertSubmenu->addSeparator();
	const auto newSequenceAction = insertSubmenu->addAction(tr("New sequence..."));
	const auto action = menu.exec(e->screenPos());
	if (!action)
		return;
	const auto offset = static_cast<size_t>(std::ceil(e->pos().x()) / kStepWidth);
	if (action == newSequenceAction)
		emit newSequenceRequested(_trackIndex, offset);
	else
		emit insertRequested(_trackIndex, offset, _composition->_tracks[_trackIndex]->_sequences[action->data().toUInt()]);
}
