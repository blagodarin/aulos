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

#include "add_voice_item.hpp"

#include "colors.hpp"
#include "utils.hpp"

#include <QPainter>

namespace
{
	constexpr auto kAddVoiceArrowHeight = kAddVoiceItemHeight * 0.25;

	QFont makeAddVoiceFont()
	{
		QFont font;
		font.setBold(true);
		font.setPixelSize(kAddVoiceItemHeight * 0.75);
		return font;
	}
}

AddVoiceItem::AddVoiceItem(QGraphicsItem* parent)
	: ButtonItem{ Mode::Click, parent }
{
}

QRectF AddVoiceItem::boundingRect() const
{
	return { 0, 0, _width, kAddVoiceItemHeight };
}

void AddVoiceItem::paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*)
{
	const auto& colors = kVoiceColors[_index % kVoiceColors.size()];
	const std::array<QPointF, 5> shape{
		QPointF{ 0, 0 },
		QPointF{ _width, 0 },
		QPointF{ _width, kAddVoiceItemHeight - kAddVoiceArrowHeight },
		QPointF{ _width / 2, kAddVoiceItemHeight },
		QPointF{ 0, kAddVoiceItemHeight - kAddVoiceArrowHeight },
	};
	if (isPressed() || isHovered())
	{
		painter->setPen(kHoverPenColor);
		painter->setBrush(isPressed() ? kPressBrushColor : kHoverBrushColor);
	}
	else
	{
		painter->setPen(Qt::transparent);
		painter->setBrush(colors._brush);
	}
	painter->drawConvexPolygon(shape.data(), static_cast<int>(shape.size()));
	painter->setPen(colors._pen);
	painter->setFont(::makeAddVoiceFont());
	painter->drawText(boundingRect(), Qt::AlignHCenter | Qt::AlignVCenter, QStringLiteral("+"));
}

void AddVoiceItem::setIndex(size_t index)
{
	_index = index;
	update();
}

void AddVoiceItem::setWidth(qreal width)
{
	prepareGeometryChange();
	_width = width;
}
