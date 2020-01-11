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

#include <array>

#include <QGraphicsSceneContextMenuEvent>
#include <QMenu>
#include <QPainter>

namespace
{
	struct FragmentColors
	{
		QColor _background;
		QColor _handle;
		QColor _outline;
	};

	const std::array<FragmentColors, 6> kFragmentColors{
		FragmentColors{ "#f88", "#f00", "#800" },
		FragmentColors{ "#ff8", "#ff0", "#880" },
		FragmentColors{ "#8f8", "#0f0", "#080" },
		FragmentColors{ "#8ff", "#0ff", "#088" },
		FragmentColors{ "#88f", "#00f", "#008" },
		FragmentColors{ "#f8f", "#f0f", "#808" },
	};
}

FragmentItem::FragmentItem(size_t trackIndex, size_t offset, size_t sequenceIndex, size_t length, QGraphicsItem* parent)
	: QGraphicsObject{ parent }
	, _trackIndex{ trackIndex }
	, _offset{ offset }
	, _sequenceIndex{ sequenceIndex }
	, _length{ length }
	, _rect{ _offset * kScaleX, _trackIndex * kScaleY, _length * kScaleX, kScaleY }
	, _colorIndex{ _trackIndex % kFragmentColors.size() }
{
}

void FragmentItem::paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*)
{
	const auto bodyLeft = _rect.left() + kScaleX / 2;
	const auto bodyRight = _rect.right() - kScaleX / 2;
	painter->setPen(kFragmentColors[_colorIndex]._outline);
	painter->setBrush(kFragmentColors[_colorIndex]._handle);
	painter->drawRect({ _rect.topLeft(), QPointF{ bodyLeft, _rect.bottom() } });
	painter->setBrush(kFragmentColors[_colorIndex]._background);
	const std::array<QPointF, 5> bodyAndTail{
		QPointF{ bodyLeft, _rect.top() },
		QPointF{ bodyRight, _rect.top() },
		QPointF{ _rect.right(), _rect.center().y() },
		QPointF{ bodyRight, _rect.bottom() },
		QPointF{ bodyLeft, _rect.bottom() },
	};
	painter->drawConvexPolygon(bodyAndTail.data(), static_cast<int>(bodyAndTail.size()));
	if (_length > 1)
	{
		auto font = painter->font();
		font.setPixelSize(kScaleY / 2);
		painter->setFont(font);
		painter->drawText(QRectF{ QPointF{ bodyLeft + kScaleX / 2, _rect.top() }, QPointF{ bodyRight, _rect.bottom() } }, Qt::AlignVCenter, QString::number(_sequenceIndex + 1));
	}
}

void FragmentItem::contextMenuEvent(QGraphicsSceneContextMenuEvent* e)
{
	QMenu menu;
	const auto editAction = menu.addAction(tr("Edit %1...").arg(_sequenceIndex + 1));
	const auto removeAction = menu.addAction(tr("Remove"));
	if (const auto action = menu.exec(e->screenPos()); action == editAction)
		emit editRequested(_trackIndex, _offset, _sequenceIndex);
	else if (action == removeAction)
		emit removeRequested(_trackIndex, _offset);
}
