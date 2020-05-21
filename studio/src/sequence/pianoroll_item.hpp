// This file is part of the Aulos toolkit.
// Copyright (C) Sergei Blagodarin.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <aulos/data.hpp>

#include <QGraphicsObject>

class PianorollItem final : public QGraphicsObject
{
	Q_OBJECT

public:
	explicit PianorollItem(QGraphicsItem* parent = nullptr);

	QRectF boundingRect() const override;
	size_t stepCount() const noexcept { return _stepCount; }
	void setStepCount(size_t count);

signals:
	void newSoundRequested(size_t offset, aulos::Note);

private:
	void mousePressEvent(QGraphicsSceneMouseEvent*) override;
	void paint(QPainter*, const QStyleOptionGraphicsItem*, QWidget*) override;

private:
	size_t _stepCount = 0;
};
