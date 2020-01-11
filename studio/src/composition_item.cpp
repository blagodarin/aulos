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

#include "composition_item.hpp"

#include "composition_scene.hpp"

#include <array>

#include <QGraphicsSceneContextMenuEvent>
#include <QMenu>
#include <QPainter>

namespace
{
	struct ItemColors
	{
		QColor _background;
		QColor _handle;
		QColor _outline;
	};

	const std::array<ItemColors, 6> kItemColors{
		ItemColors{ "#f88", "#f00", "#800" },
		ItemColors{ "#ff8", "#ff0", "#880" },
		ItemColors{ "#8f8", "#0f0", "#080" },
		ItemColors{ "#8ff", "#0ff", "#088" },
		ItemColors{ "#88f", "#00f", "#008" },
		ItemColors{ "#f8f", "#f0f", "#808" },
	};
}

CompositionItem::CompositionItem(size_t trackIndex, size_t offset, size_t sequenceIndex, size_t duration, QGraphicsItem* parent)
	: QGraphicsObject{ parent }
	, _trackIndex{ trackIndex }
	, _offset{ offset }
	, _sequenceIndex{ sequenceIndex }
	, _rect{ offset * kScaleX, trackIndex * kScaleY, duration * kScaleX, kScaleY }
	, _colorIndex{ _trackIndex % kItemColors.size() }
{
}

void CompositionItem::paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*)
{
	const auto right = _rect.right() - kScaleX / 2;
	painter->setPen(kItemColors[_colorIndex]._outline);
	if (_rect.width() > kScaleX)
	{
		painter->setBrush(kItemColors[_colorIndex]._handle);
		painter->drawRect({ _rect.topLeft(), QSizeF{ kScaleX, kScaleY } });
		painter->setBrush(kItemColors[_colorIndex]._background);
		const std::array<QPointF, 5> rightPart{
			QPointF{ _rect.left() + kScaleX, _rect.top() },
			QPointF{ right, _rect.top() },
			QPointF{ _rect.right(), _rect.center().y() },
			QPointF{ right, _rect.bottom() },
			QPointF{ _rect.left() + kScaleX, _rect.bottom() },
		};
		painter->drawConvexPolygon(rightPart.data(), static_cast<int>(rightPart.size()));
		if (_rect.width() > 2.0 * kScaleX)
		{
			auto font = painter->font();
			font.setPixelSize(kScaleY / 2);
			painter->setFont(font);
			painter->drawText(QRectF{ QPointF{ _rect.left() + 1.5 * kScaleX, _rect.top() }, QPointF{ right, _rect.bottom() } }, Qt::AlignVCenter, QString::number(_sequenceIndex + 1));
		}
	}
	else
	{
		const std::array<QPointF, 5> shape{
			QPointF{ _rect.left(), _rect.top() },
			QPointF{ right, _rect.top() },
			QPointF{ _rect.right(), _rect.center().y() },
			QPointF{ right, _rect.bottom() },
			QPointF{ _rect.left(), _rect.bottom() },
		};
		painter->setBrush(kItemColors[_colorIndex]._handle);
		painter->drawConvexPolygon(shape.data(), static_cast<int>(shape.size()));
	}
}

void CompositionItem::contextMenuEvent(QGraphicsSceneContextMenuEvent* e)
{
	QMenu menu;
	const auto submenu = menu.addMenu(tr("Sequence %1").arg(_sequenceIndex + 1));
	const auto editAction = submenu->addAction(tr("Edit..."));
	const auto removeAction = submenu->addAction(tr("Remove"));
	if (e->pos().x() > _rect.left() + kScaleX)
	{
		menu.addSeparator();
		menu.addAction(tr("Insert..."));
	}
	if (const auto action = menu.exec(e->screenPos()); action == editAction)
		emit editRequested(_trackIndex, _offset, _sequenceIndex);
	else if (action == removeAction)
		emit removeRequested(_trackIndex, _offset);
	else
		emit insertRequested(_trackIndex, _offset + static_cast<size_t>(std::ceil(e->pos().x() - _rect.left()) / kScaleX));
}
