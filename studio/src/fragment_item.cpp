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

#include "fragment_item.hpp"

#include "colors.hpp"
#include "track_item.hpp"
#include "utils.hpp"

#include <aulos/data.hpp>

#include <numeric>

#include <QGraphicsSceneContextMenuEvent>
#include <QMenu>
#include <QPainter>
#include <QTextOption>

FragmentItem::FragmentItem(TrackItem* track, size_t offset, const std::shared_ptr<aulos::SequenceData>& sequence)
	: QGraphicsObject{ track }
	, _offset{ offset }
	, _sequence{ sequence }
{
	resetSequence();
}

QRectF FragmentItem::boundingRect() const
{
	return { {}, QSizeF{ _width, kTrackHeight } };
}

void FragmentItem::paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*)
{
	const auto right = _width - kStepWidth / 2;
	const std::array<QPointF, 5> shape{
		QPointF{ 0, 0 },
		QPointF{ right, 0 },
		QPointF{ _width, kTrackHeight / 2 },
		QPointF{ right, kTrackHeight },
		QPointF{ 0, kTrackHeight },
	};
	const auto& colors = kFragmentColors[static_cast<const TrackItem*>(parentItem())->trackIndex() % kFragmentColors.size()];
	painter->setPen(colors._pen);
	painter->setBrush(colors._brush);
	painter->drawConvexPolygon(shape.data(), static_cast<int>(shape.size()));
	if (_length > 0)
	{
		constexpr auto fontSize = kTrackHeight * 0.75;
		constexpr auto xOffset = (kTrackHeight - fontSize) / 2.0;
		constexpr auto xScale = 7.0 / 16.0;
		painter->save();
		auto font = painter->font();
		font.setPixelSize(fontSize);
		painter->setFont(font);
		painter->setTransform(QTransform::fromScale(xScale, 1.0), true);
		_name.prepare(painter->transform(), font);
		const QPointF topLeft{ xOffset / xScale, (kTrackHeight - _name.size().height()) / 2 };
		painter->setClipRect(QRectF{ { topLeft.x(), 0 }, QPointF{ right / xScale, kTrackHeight } });
		painter->drawStaticText(topLeft, _name);
		painter->restore();
	}
}

bool FragmentItem::updateSequence(const std::shared_ptr<aulos::SequenceData>& sequence)
{
	if (_sequence != sequence)
		return false;
	resetSequence();
	return true;
}

void FragmentItem::contextMenuEvent(QGraphicsSceneContextMenuEvent* e)
{
	emit fragmentMenuRequested(_offset, e->screenPos());
}

void FragmentItem::mouseDoubleClickEvent(QGraphicsSceneMouseEvent*)
{
	emit fragmentActionRequested(_offset);
}

void FragmentItem::resetSequence()
{
	prepareGeometryChange();
	_length = _sequence->_sounds.empty() ? 0 : std::reduce(_sequence->_sounds.begin(), _sequence->_sounds.end(), size_t{ 1 }, [](size_t length, const aulos::Sound& sound) { return length + sound._delay; });
	_width = (_length + 0.5) * kStepWidth;
	if (_length > 0)
	{
		QTextOption textOption;
		textOption.setWrapMode(QTextOption::NoWrap);
		_name.setText(::makeSequenceName(*_sequence, true));
		_name.setTextFormat(Qt::RichText);
		_name.setTextOption(textOption);
		_name.setTextWidth((_length - 1) * kStepWidth);
	}
	else
		_name = {};
	setToolTip(::makeSequenceName(*_sequence));
}
