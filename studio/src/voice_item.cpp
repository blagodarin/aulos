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

#include <array>

#include <QPainter>

namespace
{
	constexpr auto kFontSize = kTrackHeight * 0.5;
	constexpr auto kMargin = (kTrackHeight - kFontSize) / 2;
	constexpr auto kAddVoiceHeight = kTrackHeight * 0.75;
	constexpr auto kAddVoiceFontSize = kAddVoiceHeight * 0.75;

	struct VoiceColors
	{
		QColor _brush;
		QColor _pen;
	};

	const std::array<VoiceColors, 2> kVoiceColors{
		VoiceColors{ "#fff", "#000" },
		VoiceColors{ "#ddd", "#000" },
	};

	QFont makeVoiceFont(bool addVoice)
	{
		QFont font;
		font.setBold(addVoice);
		font.setPixelSize(addVoice ? kAddVoiceFontSize : kFontSize);
		return font;
	}
}

VoiceItem::VoiceItem(const std::shared_ptr<const aulos::Voice>& voice, QGraphicsItem* parent)
	: QGraphicsItem{ parent }
	, _voice{ voice }
{
	QTextOption textOption;
	textOption.setWrapMode(QTextOption::NoWrap);
	_name.setText(_voice ? QString::fromStdString(_voice->_name) : QStringLiteral("+"));
	_name.setTextFormat(Qt::PlainText);
	_name.setTextOption(textOption);
	_name.prepare({}, ::makeVoiceFont(!_voice));
}

QRectF VoiceItem::boundingRect() const
{
	return { { -_width, 0 }, QSizeF{ _width, _voice ? kTrackHeight : kAddVoiceHeight } };
}

void VoiceItem::paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*)
{
	const auto& colors = kVoiceColors[_index % kVoiceColors.size()];
	painter->setBrush(colors._brush);
	painter->setPen(Qt::transparent);
	if (!_voice)
	{
		const auto arrowHeight = kAddVoiceHeight / 2;
		const std::array<QPointF, 5> shape{
			QPointF{ -_width, 0 },
			QPointF{ 0, 0 },
			QPointF{ 0, arrowHeight },
			QPointF{ -_width / 2, kAddVoiceHeight },
			QPointF{ -_width, arrowHeight },
		};
		painter->drawConvexPolygon(shape.data(), static_cast<int>(shape.size()));
	}
	else
		painter->drawRect(boundingRect());
	painter->setPen(colors._pen);
	painter->setFont(::makeVoiceFont(!_voice));
	const auto nameSize = _name.size();
	painter->drawStaticText(QPointF{ _voice ? kMargin - _width : -(_width + nameSize.width()) / 2, _voice ? (kTrackHeight - nameSize.height()) / 2 : 0 }, _name);
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
