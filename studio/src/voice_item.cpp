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

	struct VoiceColors
	{
		QColor _brush;
		QColor _pen;
	};

	const std::array<VoiceColors, 2> kVoiceColors{
		VoiceColors{ "#fff", "#000" },
		VoiceColors{ "#ddd", "#000" },
	};

	QFont defaultFont()
	{
		QFont font;
		font.setPixelSize(kFontSize);
		return font;
	}
}

VoiceItem::VoiceItem(const std::shared_ptr<const aulos::CompositionData>& composition, size_t trackIndex, QGraphicsItem* parent)
	: QGraphicsItem{ parent }
	, _composition{ composition }
	, _trackIndex{ trackIndex }
	, _rect{ 0, _trackIndex * kTrackHeight, 0, kTrackHeight }
{
	QTextOption textOption;
	textOption.setWrapMode(QTextOption::NoWrap);
	_name.setText(QString::fromStdString(_composition->_tracks[_trackIndex]._voice->_name));
	_name.setTextFormat(Qt::PlainText);
	_name.setTextOption(textOption);
	_name.prepare({}, ::defaultFont());
}

void VoiceItem::paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*)
{
	const auto& colors = kVoiceColors[_trackIndex % kVoiceColors.size()];
	painter->setBrush(colors._brush);
	painter->setPen(Qt::transparent);
	painter->drawRect(_rect);
	painter->setPen(colors._pen);
	painter->setFont(::defaultFont());
	painter->drawStaticText(QPointF{ _rect.left() + kMargin, _rect.top() + (kTrackHeight - _name.size().height()) / 2 }, _name);
}

qreal VoiceItem::requiredWidth() const
{
	return kMargin + _name.size().width() + kMargin;
}

void VoiceItem::setWidth(qreal width)
{
	prepareGeometryChange();
	_rect.setLeft(-width);
}
