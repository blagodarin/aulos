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

#include "voice_item.hpp"

#include "../theme.hpp"

#include <QGraphicsSceneEvent>
#include <QMenu>
#include <QPainter>

namespace
{
	QFont makeVoiceFont()
	{
		QFont font;
		font.setPixelSize(kVoiceNameFontSize);
		return font;
	}
}

VoiceItem::VoiceItem(const void* id, QGraphicsItem* parent)
	: QGraphicsObject{ parent }
	, _voiceId{ id }
{
}

QRectF VoiceItem::boundingRect() const
{
	return { 0, 0, _width, _trackCount * kTrackHeight };
}

void VoiceItem::paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*)
{
	const auto& colors = _highlighted ? kVoiceHighlightColors[_index % kVoiceHighlightColors.size()] : kVoiceColors[_index % kVoiceColors.size()];
	QPen pen{ _highlighted ? colors._pen : Qt::transparent };
	pen.setWidth(_highlighted ? 3 : 0);
	painter->setPen(pen);
	painter->setBrush(colors._brush);
	painter->drawRect(boundingRect());
	painter->setPen(colors._pen);
	painter->setFont(::makeVoiceFont());
	const auto nameSize = _name.size();
	painter->drawStaticText(QPointF{ kVoiceNameMargin, (_trackCount * kTrackHeight - nameSize.height()) / 2 }, _name);
}

qreal VoiceItem::requiredWidth() const
{
	return kVoiceNameMargin + _name.size().width() + kVoiceNameMargin;
}

void VoiceItem::setHighlighted(bool highlighted)
{
	_highlighted = highlighted;
	update();
}

void VoiceItem::setTrackCount(size_t count)
{
	prepareGeometryChange();
	_trackCount = count;
}

void VoiceItem::setVoiceIndex(size_t index)
{
	_index = index;
	update();
}

void VoiceItem::setVoiceName(const QString& name)
{
	QTextOption textOption;
	textOption.setWrapMode(QTextOption::NoWrap);
	_name.setText(name);
	_name.setTextFormat(Qt::PlainText);
	_name.setTextOption(textOption);
	_name.prepare({}, ::makeVoiceFont());
	update();
}

void VoiceItem::setWidth(qreal width)
{
	prepareGeometryChange();
	_width = width;
}

void VoiceItem::contextMenuEvent(QGraphicsSceneContextMenuEvent* e)
{
	emit voiceMenuRequested(_voiceId, e->screenPos());
}

void VoiceItem::mouseDoubleClickEvent(QGraphicsSceneMouseEvent*)
{
	emit voiceActionRequested(_voiceId);
}

void VoiceItem::mousePressEvent(QGraphicsSceneMouseEvent* e)
{
	if (e->button() == Qt::LeftButton)
		emit voiceSelected(_voiceId);
	QGraphicsObject::mousePressEvent(e);
}
