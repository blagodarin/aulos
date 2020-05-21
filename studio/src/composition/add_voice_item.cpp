// This file is part of the Aulos toolkit.
// Copyright (C) Sergei Blagodarin.
// SPDX-License-Identifier: Apache-2.0

#include "add_voice_item.hpp"

#include "../theme.hpp"

#include <QPainter>

namespace
{
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
