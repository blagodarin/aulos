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

#include "utils.hpp"

#include <array>

#include <QPainter>

namespace
{
	const std::array<QColor, 2> kTimelineColors{ "#fff", "#ddd" };
}

TimelineItem::TimelineItem(size_t speed, size_t length, QGraphicsItem* parent)
	: QGraphicsItem{ parent }
	, _speed{ speed }
	, _length{ length }
	, _rect{ 0.0, -kTimelineHeight, _length * kStepWidth, kTimelineHeight }
{
}

void TimelineItem::paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*)
{
	size_t index = 0;
	QRectF rect{ _rect.topLeft(), QSizeF{ _speed * kStepWidth, kTimelineHeight } };
	constexpr auto fontSize = kTimelineHeight * 0.75;
	constexpr auto textOffset = (kTimelineHeight - fontSize) / 2.0;
	auto font = painter->font();
	font.setPixelSize(fontSize);
	painter->setFont(font);
	while (index < _length / _speed)
	{
		painter->setPen(Qt::transparent);
		painter->setBrush(kTimelineColors[index++ % kTimelineColors.size()]);
		painter->drawRect(rect);
		painter->setPen(Qt::black);
		painter->drawText(rect.adjusted(-textOffset, 0.0, -textOffset, 0.0), Qt::AlignRight | Qt::AlignVCenter, QString::number(index));
		rect.moveLeft(rect.right());
	}
	if (const auto remainder = _length % _speed)
	{
		rect.setRight(_rect.right());
		painter->setPen(Qt::transparent);
		painter->setBrush(kTimelineColors[index % kTimelineColors.size()]);
		painter->drawRect(rect);
	}
}

void TimelineItem::setCompositionSpeed(size_t speed)
{
	if (_speed == speed)
		return;
	_speed = speed;
}

void TimelineItem::setTrackLength(size_t length)
{
	if (_length == length)
		return;
	prepareGeometryChange();
	_length = length;
	_rect.setWidth(_length * kStepWidth);
}
