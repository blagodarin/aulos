// This file is part of the Aulos toolkit.
// Copyright (C) Sergei Blagodarin.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <aulos/data.hpp>

#include <QGraphicsObject>

class PianorollItem;

class SoundItem final : public QGraphicsObject
{
	Q_OBJECT

public:
	SoundItem(size_t offset, aulos::Note, QGraphicsItem* parent = nullptr);

	aulos::Note note() const noexcept { return _note; }
	size_t offset() const noexcept { return _offset; }
	void setNote(aulos::Note);

signals:
	void playRequested();
	void removeRequested();

private:
	QRectF boundingRect() const override;
	void mousePressEvent(QGraphicsSceneMouseEvent*) override;
	void paint(QPainter*, const QStyleOptionGraphicsItem*, QWidget*) override;

private:
	const size_t _offset;
	aulos::Note _note;
};
