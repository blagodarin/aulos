// This file is part of the Aulos toolkit.
// Copyright (C) Sergei Blagodarin.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <QGraphicsItem>

class CursorItem final : public QGraphicsItem
{
public:
	explicit CursorItem(QGraphicsItem* parent = nullptr);

	QRectF boundingRect() const override;
	void paint(QPainter*, const QStyleOptionGraphicsItem*, QWidget*) override;
	void setTrackCount(size_t);

private:
	size_t _trackCount = 0;
};
