// This file is part of the Aulos toolkit.
// Copyright (C) Sergei Blagodarin.
// SPDX-License-Identifier: Apache-2.0

#include "fragment_item.hpp"

#include "../theme.hpp"

#include <seir_synth/data.hpp>

#include <numeric>

#include <QGraphicsSceneEvent>
#include <QMenu>
#include <QPainter>
#include <QTextOption>

FragmentItem::FragmentItem(size_t trackIndex, size_t offset, const void* sequenceId, QGraphicsItem* parent)
	: QGraphicsObject{ parent }
	, _trackIndex{ trackIndex }
	, _offset{ offset }
	, _sequenceId{ sequenceId }
{
	_polygon.reserve(5);
}

QRectF FragmentItem::boundingRect() const
{
	return { 0, 0, _width + kFragmentArrowWidth, kTrackHeight };
}

void FragmentItem::paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*)
{
	const auto& colors = _highlighted ? kFragmentHighlightColors[_trackIndex % kFragmentHighlightColors.size()] : kFragmentColors[_trackIndex % kFragmentColors.size()];
	QPen pen{ colors._pen };
	pen.setWidth(_selected ? 3 : 0);
	painter->setPen(pen);
	painter->setBrush(colors._brush);
	painter->drawConvexPolygon(_polygon);
	if (!_sounds.empty())
	{
		constexpr auto xScale = 7.0 / 16.0;
		painter->save();
		auto font = painter->font();
		font.setPixelSize(kFragmentFontSize);
		painter->setFont(font);
		painter->setTransform(QTransform::fromScale(xScale, 1.0), true);
		QPointF topLeft{ 1 / xScale, (kTrackHeight - QFontMetricsF{ font }.height()) / 2 };
		QStaticText sustain{ "-" };
		for (const auto& sound : _sounds)
		{
			topLeft.rx() += sound._delay * kStepWidth / xScale;
			painter->drawStaticText(topLeft, *sound._text);
			for (size_t i = 0; i < sound._sustain; ++i)
				painter->drawStaticText(QPointF(topLeft.x() + (i + 1) * kStepWidth / xScale, topLeft.y()), sustain);
		}
		painter->restore();
	}
}

void FragmentItem::setHighlighted(bool highlighted, bool selected)
{
	if (!highlighted)
		selected = false;
	if (_highlighted == highlighted && _selected == selected)
		return;
	_highlighted = highlighted;
	_selected = selected;
	update();
}

void FragmentItem::setSequence(const std::vector<FragmentSound>& sounds)
{
	prepareGeometryChange();
	_sounds = sounds;
	if (!sounds.empty())
	{
		_length = std::accumulate(sounds.begin(), sounds.end(), size_t{ 1 }, [](size_t length, const FragmentSound& sound) { return length + sound._delay; });
		auto end = std::find_if(sounds.rbegin(), sounds.rend(), [](const FragmentSound& sound) { return sound._delay != 0; });
		if (end != sounds.rend())
			++end;
		_length += std::max_element(sounds.rbegin(), end, [](const FragmentSound& a, const FragmentSound& b) { return a._sustain < b._sustain; })->_sustain;
	}
	else
		_length = 0;
	_width = _length * kStepWidth;
	_polygon.clear();
	_polygon << QPointF{ 0, 0 } << QPointF{ _width, 0 } << QPointF{ _width + kFragmentArrowWidth, kTrackHeight / 2 } << QPointF{ _width, kTrackHeight } << QPointF{ 0, kTrackHeight };
}

void FragmentItem::setTrackIndex(size_t index)
{
	_trackIndex = index;
	update();
}

void FragmentItem::contextMenuEvent(QGraphicsSceneContextMenuEvent* e)
{
	e->setAccepted(_polygon.containsPoint(e->pos(), Qt::OddEvenFill));
	if (e->isAccepted())
		emit fragmentMenuRequested(_offset, e->screenPos());
}

void FragmentItem::mousePressEvent(QGraphicsSceneMouseEvent* e)
{
	if (e->button() == Qt::LeftButton)
		emit fragmentSelected(_sequenceId, _offset);
	QGraphicsObject::mousePressEvent(e);
}
