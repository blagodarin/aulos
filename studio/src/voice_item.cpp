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

#include "utils.hpp"

#include <aulos/data.hpp>

#include <QPainter>

namespace
{
	constexpr auto kFontSize = kTrackHeight * 0.5;
	constexpr auto kMargin = (kTrackHeight - kFontSize) / 2;

	QFont makeVoiceFont()
	{
		QFont font;
		font.setPixelSize(kFontSize);
		return font;
	}
}

VoiceItem::VoiceItem(const std::shared_ptr<aulos::Voice>& voice, QGraphicsItem* parent)
	: QGraphicsItem{ parent }
	, _voice{ voice }
{
	QTextOption textOption;
	textOption.setWrapMode(QTextOption::NoWrap);
	_name.setText(QString::fromStdString(_voice->_name));
	_name.setTextFormat(Qt::PlainText);
	_name.setTextOption(textOption);
	_name.prepare({}, ::makeVoiceFont());
}

QRectF VoiceItem::boundingRect() const
{
	return { { -_width, 0 }, QSizeF{ _width, kTrackHeight } };
}

void VoiceItem::paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*)
{
	const auto& colors = kVoiceColors[_index % kVoiceColors.size()];
	painter->setBrush(colors._brush);
	painter->setPen(Qt::transparent);
	painter->drawRect(boundingRect());
	painter->setPen(colors._pen);
	painter->setFont(::makeVoiceFont());
	const auto nameSize = _name.size();
	painter->drawStaticText(QPointF{ kMargin - _width, (kTrackHeight - nameSize.height()) / 2 }, _name);
}

qreal VoiceItem::requiredWidth() const
{
	return kMargin + _name.size().width() + kMargin;
}

void VoiceItem::setIndex(size_t index)
{
	_index = index;
	update();
}

void VoiceItem::setWidth(qreal width)
{
	prepareGeometryChange();
	_width = width;
}
