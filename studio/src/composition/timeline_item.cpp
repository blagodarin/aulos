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

#include "timeline_item.hpp"

#include "../theme.hpp"

#include <cmath>

#include <QGraphicsSceneEvent>
#include <QPainter>
#include <QStyleOptionGraphicsItem>

TimelineItem::TimelineItem(QGraphicsItem* parent)
	: QGraphicsItem{ parent }
{
	setFlag(QGraphicsItem::ItemUsesExtendedStyleOption);

	const auto offsetMarkSize = kCompositionHeaderHeight - kTimelineHeight;
	_offsetMark.reserve(3);
	_offsetMark << QPointF{ 0, 0 } << QPointF{ kStepWidth, offsetMarkSize / 2 } << QPointF{ 0, offsetMarkSize };
}

QRectF TimelineItem::boundingRect() const
{
	return { 0, 0, _length * kStepWidth, kCompositionHeaderHeight };
}

void TimelineItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget*)
{
	painter->save();
	painter->translate(QPointF{ _offset * kStepWidth, 0 });
	painter->setPen(kTimelineOffsetMarkColors._pen);
	painter->setBrush(kTimelineOffsetMarkColors._brush);
	painter->drawConvexPolygon(_offsetMark);
	painter->restore();

	size_t index = 0;
	QRectF rect{ 0, kCompositionHeaderHeight - kTimelineHeight, _speed * kStepWidth, kTimelineHeight };
	constexpr auto textOffset = (kTimelineHeight - kTimelineFontSize) / 2.0;
	auto font = painter->font();
	font.setPixelSize(kTimelineFontSize);
	painter->setFont(font);
	while (index < _length / _speed)
	{
		if (rect.left() > option->exposedRect.right())
			return;
		if (rect.right() >= option->exposedRect.left())
		{
			const auto& colors = kTimelineColors[index % kTimelineColors.size()];
			painter->setPen(Qt::transparent);
			painter->setBrush(colors._brush);
			painter->drawRect(rect);
			painter->setPen(colors._pen);
			painter->drawText(rect.adjusted(-textOffset, 0.0, -textOffset, 0.0), Qt::AlignRight | Qt::AlignVCenter, QString::number(index + 1));
		}
		rect.moveLeft(rect.right());
		++index;
	}
	if (_length % _speed && rect.left() <= option->exposedRect.right() && rect.right() >= option->exposedRect.left())
	{
		rect.setRight(_length * kStepWidth);
		painter->setPen(Qt::transparent);
		painter->setBrush(kTimelineColors[index % kTimelineColors.size()]._brush);
		painter->drawRect(rect);
	}
}

void TimelineItem::setCompositionLength(size_t length)
{
	prepareGeometryChange();
	_length = length;
}

void TimelineItem::setCompositionOffset(size_t offset)
{
	_offset = offset;
	update();
}

void TimelineItem::setCompositionSpeed(unsigned speed)
{
	_speed = speed;
	update();
}

void TimelineItem::mousePressEvent(QGraphicsSceneMouseEvent* e)
{
	if (e->button() == Qt::LeftButton)
		setCompositionOffset(static_cast<size_t>(std::ceil(e->pos().x()) / kStepWidth));
	QGraphicsItem::mousePressEvent(e);
}
