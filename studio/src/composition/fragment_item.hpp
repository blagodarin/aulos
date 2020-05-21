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

	FragmentSound(size_t delay, const std::shared_ptr<QStaticText>& text)
		: _delay{ delay }, _text{ text } {}
};

class FragmentItem final : public QGraphicsObject
{
	Q_OBJECT

public:
	FragmentItem(size_t trackIndex, size_t offset, const void* sequenceId, QGraphicsItem* parent);

	QRectF boundingRect() const override;
	size_t fragmentLength() const noexcept { return _length; }
	size_t fragmentOffset() const noexcept { return _offset; }
	bool isHighlighted() const noexcept { return _highlighted; }
	void paint(QPainter*, const QStyleOptionGraphicsItem*, QWidget*) override;
	const void* sequenceId() const noexcept { return _sequenceId; }
	void setHighlighted(bool);
	void setSequence(const std::vector<FragmentSound>&);
	void setTrackIndex(size_t index);

signals:
	void fragmentMenuRequested(size_t offset, const QPoint& pos);
	void sequenceSelected(const void* sequenceId);

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
};
