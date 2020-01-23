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

#include "add_time_item.hpp"

#include "utils.hpp"

#include <array>

#include <QPainter>

namespace
{
	constexpr auto kAddTimeArrowWidth = kAddTimeItemWidth * 0.25;

	const QColor kHoverPenColor{ "#07f" };
	const QColor kHoverBrushColor{ "#cff" };
	const QColor kPressBrushColor{ "#8cf" };
}

AddTimeItem::AddTimeItem(const QColor& color, QGraphicsItem* parent)
	: ButtonItem{ parent }
	, _color{ color }
{
}

QRectF AddTimeItem::boundingRect() const
{
	return { 0, -kTimelineHeight, _extraLength * kStepWidth + kAddTimeItemWidth, kTimelineHeight };
}

void AddTimeItem::paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*)
{
	const auto rect = boundingRect();

	const std::array<QPointF, 5> shape{
		rect.topLeft(),
		QPointF{ rect.right() - kAddTimeArrowWidth, rect.top() },
		QPointF{ rect.right(), rect.center().y() },
		QPointF{ rect.right() - kAddTimeArrowWidth, rect.bottom() },
		rect.bottomLeft(),
	};
	if (isPressed() || isHovered())
	{
		painter->setPen(kHoverPenColor);
		painter->setBrush(isPressed() ? kPressBrushColor : kHoverBrushColor);
	}
	else
	{
		painter->setPen(Qt::transparent);
		painter->setBrush(_color);
	}
	painter->drawConvexPolygon(shape.data(), static_cast<int>(shape.size()));

	constexpr auto fontSize = kTimelineHeight * 0.75;
	auto font = painter->font();
	font.setBold(true);
	font.setPixelSize(fontSize);
	painter->setFont(font);
	painter->setPen(isPressed() || isHovered() ? kHoverPenColor : Qt::black);
	painter->drawText(rect.adjusted(rect.right() - kAddTimeItemWidth, 0, 0, 0), Qt::AlignHCenter | Qt::AlignVCenter, QStringLiteral("+"));
}

void AddTimeItem::setGeometry(const QColor& color, size_t extraLength)
{
	prepareGeometryChange();
	_color = color;
	_extraLength = extraLength;
}
