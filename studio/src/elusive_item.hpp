// This file is part of the Aulos toolkit.
// Copyright (C) Sergei Blagodarin.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <QGraphicsObject>

class ElusiveItem final : public QGraphicsObject
{
	Q_OBJECT

public:
	explicit ElusiveItem(QGraphicsItem* parent = nullptr);

	void setHeight(qreal);

signals:
	void elude();

private:
	QRectF boundingRect() const override;
	void paint(QPainter*, const QStyleOptionGraphicsItem*, QWidget*) override;

private:
	qreal _height = 1;
};
