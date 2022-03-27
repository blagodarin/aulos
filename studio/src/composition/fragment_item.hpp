// This file is part of the Aulos toolkit.
// Copyright (C) Sergei Blagodarin.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <memory>
#include <vector>

#include <QGraphicsObject>

class QStaticText;

struct FragmentSound
{
	size_t _delay = 0;
	std::shared_ptr<QStaticText> _text;
	size_t _sustain = 0;

	FragmentSound(size_t delay, const std::shared_ptr<QStaticText>& text, size_t sustain)
		: _delay{ delay }, _text{ text }, _sustain{ sustain } {}
};

class FragmentItem final : public QGraphicsObject
{
	Q_OBJECT

public:
	FragmentItem(size_t trackIndex, size_t offset, const void* sequenceId, QGraphicsItem* parent);

	QRectF boundingRect() const override;
	size_t fragmentLength() const noexcept { return _length; }
	size_t fragmentOffset() const noexcept { return _offset; }
	void paint(QPainter*, const QStyleOptionGraphicsItem*, QWidget*) override;
	const void* sequenceId() const noexcept { return _sequenceId; }
	void setHighlighted(bool highlighted, bool selected);
	void setSequence(const std::vector<FragmentSound>&);
	void setTrackIndex(size_t index);

signals:
	void fragmentMenuRequested(size_t offset, const QPoint& pos);
	void fragmentSelected(const void* sequenceId, size_t offset);

private:
	void contextMenuEvent(QGraphicsSceneContextMenuEvent*) override;
	void mousePressEvent(QGraphicsSceneMouseEvent*) override;

private:
	size_t _trackIndex;
	const size_t _offset;
	const void* const _sequenceId;
	std::vector<FragmentSound> _sounds;
	size_t _length = 0;
	qreal _width = 0;
	QPolygonF _polygon;
	bool _highlighted = false;
	bool _selected = false;
};
