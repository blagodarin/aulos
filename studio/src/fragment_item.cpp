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

#include "track_item.hpp"
#include "utils.hpp"

#include <aulos/data.hpp>

#include <array>
#include <numeric>

#include <QGraphicsSceneContextMenuEvent>
#include <QMenu>
#include <QPainter>
#include <QTextOption>

namespace
{
	struct FragmentColors
	{
		QColor _brush;
		QColor _pen;
	};

	const std::array<FragmentColors, 6> kFragmentColors{
		FragmentColors{ "#f88", "#400" },
		FragmentColors{ "#ff8", "#440" },
		FragmentColors{ "#8f8", "#040" },
		FragmentColors{ "#8ff", "#044" },
		FragmentColors{ "#88f", "#004" },
		FragmentColors{ "#f8f", "#404" },
	};
}

FragmentItem::FragmentItem(TrackItem* track, size_t offset, const std::shared_ptr<const aulos::SequenceData>& sequence)
	: QGraphicsObject{ track }
	, _offset{ offset }
	, _sequence{ sequence }
	, _length{ std::reduce(sequence->_sounds.begin(), sequence->_sounds.end(), size_t{}, [](size_t length, const aulos::Sound& sound) { return length + sound._pause; }) }
	, _rect{ _offset * kStepWidth, track->trackIndex() * kTrackHeight, std::max<size_t>(_length, 1) * kStepWidth, kTrackHeight }
{
	setToolTip(::makeSequenceName(*_sequence));
	if (_length > 1)
	{
		QTextOption textOption;
		textOption.setWrapMode(QTextOption::NoWrap);
		_name.setText(::makeSequenceName(*_sequence, true));
		_name.setTextFormat(Qt::RichText);
		_name.setTextOption(textOption);
		_name.setTextWidth((_length - 1) * kStepWidth);
	}
}

void FragmentItem::paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*)
{
	const auto right = _rect.right() - kStepWidth / 2;
	const std::array<QPointF, 5> shape{
		_rect.topLeft(),
		QPointF{ right, _rect.top() },
		QPointF{ _rect.right(), _rect.center().y() },
		QPointF{ right, _rect.bottom() },
		_rect.bottomLeft(),
	};
	const auto& colors = kFragmentColors[static_cast<const TrackItem*>(parentItem())->trackIndex() % kFragmentColors.size()];
	painter->setPen(colors._pen);
	painter->setBrush(colors._brush);
	painter->drawConvexPolygon(shape.data(), static_cast<int>(shape.size()));
	if (_length > 1)
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
		const QPointF topLeft{ (_rect.left() + xOffset) / xScale, _rect.top() + (kTrackHeight - _name.size().height()) / 2 };
		painter->setClipRect(QRectF{ { topLeft.x(), _rect.top() }, QPointF{ right / xScale, _rect.bottom() } });
		painter->drawStaticText(topLeft, _name);
		painter->restore();
	}
}

void FragmentItem::contextMenuEvent(QGraphicsSceneContextMenuEvent* e)
{
	QMenu menu;
	const auto editAction = menu.addAction(tr("Edit..."));
	const auto removeAction = menu.addAction(tr("Remove"));
	if (const auto action = menu.exec(e->screenPos()))
	{
		const auto trackIndex = static_cast<const TrackItem*>(parentItem())->trackIndex();
		if (action == editAction)
			emit editRequested(trackIndex, _offset, _sequence);
		else if (action == removeAction)
			emit removeRequested(trackIndex, _offset);
	}
}
