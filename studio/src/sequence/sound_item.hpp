// This file is part of the Aulos toolkit.
// Copyright (C) Sergei Blagodarin.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <seir_synth/data.hpp>

#include <QGraphicsObject>

class PianorollItem;

class SoundItem final : public QGraphicsObject
{
	Q_OBJECT

public:
	SoundItem(size_t offset, seir::synth::Note, QGraphicsItem* parent = nullptr);

	seir::synth::Note note() const noexcept { return _note; }
	size_t offset() const noexcept { return _offset; }
	void setNote(seir::synth::Note);

signals:
	void playRequested();
	void removeRequested();

private:
	QRectF boundingRect() const override;
	void mousePressEvent(QGraphicsSceneMouseEvent*) override;
	void paint(QPainter*, const QStyleOptionGraphicsItem*, QWidget*) override;

private:
	const size_t _offset;
	seir::synth::Note _note;
};
