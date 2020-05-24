// This file is part of the Aulos toolkit.
// Copyright (C) Sergei Blagodarin.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <QGraphicsObject>

class TimelineItem final : public QGraphicsObject
{
	Q_OBJECT

public:
	explicit TimelineItem(QGraphicsItem* parent = nullptr);

	QRectF boundingRect() const override;
	void paint(QPainter*, const QStyleOptionGraphicsItem*, QWidget*) override;
	void setCompositionLength(size_t length);
	void setCompositionOffset(size_t offset);
	void setCompositionSpeed(unsigned speed);
	size_t compositionLength() const noexcept { return _length; }
	size_t compositionOffset() const noexcept { return _offset; }
	unsigned compositionSpeed() const noexcept { return _speed; }

signals:
	void menuRequested(size_t offset, const QPoint& pos);

private:
	void contextMenuEvent(QGraphicsSceneContextMenuEvent*) override;
	void mousePressEvent(QGraphicsSceneMouseEvent*) override;

private:
	unsigned _speed = 1;
	size_t _length = 0;
	size_t _offset = 0;
	QPolygonF _offsetMark;
};
