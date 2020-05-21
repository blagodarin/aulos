// This file is part of the Aulos toolkit.
// Copyright (C) Sergei Blagodarin.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <QGraphicsObject>

class TrackItem final : public QGraphicsObject
{
	Q_OBJECT

public:
	explicit TrackItem(const void* id, QGraphicsItem* parent = nullptr);

	QRectF boundingRect() const override;
	bool isFirstTrack() const noexcept { return _first; }
	void setFirstTrack(bool);
	void setTrackIndex(size_t);
	void setTrackLength(size_t);
	const void* trackId() const noexcept { return _trackId; }
	size_t trackIndex() const noexcept { return _index; }

signals:
	void trackActionRequested(const void* trackId);
	void trackMenuRequested(const void* trackId, size_t offset, const QPoint& pos);

private:
	void contextMenuEvent(QGraphicsSceneContextMenuEvent*) override;
	void mouseDoubleClickEvent(QGraphicsSceneMouseEvent*) override;
	void paint(QPainter*, const QStyleOptionGraphicsItem*, QWidget*) override;

private:
	const void* const _trackId;
	size_t _length = 0;
	size_t _index = 0;
	bool _first = false;
};
