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

#include "composition_scene.hpp"
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
		QColor _background;
		QColor _outline;
	};

	const std::array<FragmentColors, 6> kFragmentColors{
		FragmentColors{ "#f88", "#800" },
		FragmentColors{ "#ff8", "#880" },
		FragmentColors{ "#8f8", "#080" },
		FragmentColors{ "#8ff", "#088" },
		FragmentColors{ "#88f", "#008" },
		FragmentColors{ "#f8f", "#808" },
	};
}

FragmentItem::FragmentItem(size_t trackIndex, size_t offset, const std::shared_ptr<const aulos::SequenceData>& sequence, QGraphicsItem* parent)
	: QGraphicsObject{ parent }
	, _trackIndex{ trackIndex }
	, _offset{ offset }
	, _sequence{ sequence }
	, _length{ std::reduce(sequence->_sounds.begin(), sequence->_sounds.end(), size_t{}, [](size_t length, const aulos::Sound& sound) { return length + sound._pause; }) }
	, _rect{ _offset * kScaleX, _trackIndex * kScaleY, std::max<size_t>(_length, 1) * kScaleX, kScaleY }
	, _colorIndex{ _trackIndex % kFragmentColors.size() }
{
	setToolTip(::makeSequenceName(*_sequence));
	if (_length > 1)
	{
		QTextOption textOption{ Qt::AlignVCenter };
		textOption.setWrapMode(QTextOption::NoWrap);
		_name.setText(::makeSequenceName(*_sequence, true));
		_name.setTextFormat(Qt::RichText);
		_name.setTextOption(textOption);
		_name.setTextWidth((_length - 1) * kScaleX);
	}
}

void FragmentItem::paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*)
{
	painter->setPen(kFragmentColors[_colorIndex]._outline);
	const auto right = _rect.right() - kScaleX / 2;
	const std::array<QPointF, 5> shape{
		_rect.topLeft(),
		QPointF{ right, _rect.top() },
		QPointF{ _rect.right(), _rect.center().y() },
		QPointF{ right, _rect.bottom() },
		_rect.bottomLeft(),
	};
	painter->setBrush(kFragmentColors[_colorIndex]._background);
	painter->drawConvexPolygon(shape.data(), static_cast<int>(shape.size()));
	if (_length > 1)
	{
		auto font = painter->font();
		font.setPixelSize(kScaleY / 2);
		painter->setFont(font);
		painter->save();
		painter->setTransform(QTransform::fromScale(0.5, 1.0), true);
		const QPointF topLeft{ 2.0 * _rect.left() + kScaleX, _rect.top() + 0.17 * kScaleY };
		painter->setClipRect(QRectF{ topLeft, QPointF{ 2.0 * right, _rect.bottom() } });
		painter->drawStaticText(topLeft, _name);
		painter->restore();
	}
}

void FragmentItem::contextMenuEvent(QGraphicsSceneContextMenuEvent* e)
{
	QMenu menu;
	const auto editAction = menu.addAction(tr("Edit..."));
	const auto removeAction = menu.addAction(tr("Remove"));
	if (const auto action = menu.exec(e->screenPos()); action == editAction)
		emit editRequested(_trackIndex, _offset, _sequence);
	else if (action == removeAction)
		emit removeRequested(_trackIndex, _offset);
}
