// This file is part of the Aulos toolkit.
// Copyright (C) Sergei Blagodarin.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <QGraphicsObject>

class LoopItem final : public QGraphicsObject
{
	Q_OBJECT

public:
	explicit LoopItem(QGraphicsItem* parent = nullptr);

	QRectF boundingRect() const override;
	void paint(QPainter*, const QStyleOptionGraphicsItem*, QWidget*) override;
	void setLoopLength(size_t);

signals:
	void menuRequested(const QPoint& pos);

private:
	void contextMenuEvent(QGraphicsSceneContextMenuEvent*) override;

private:
	size_t _loopLength = 0;
};
