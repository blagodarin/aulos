// This file is part of the Aulos toolkit.
// Copyright (C) Sergei Blagodarin.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <QGraphicsObject>
#include <QStaticText>

class VoiceItem final : public QGraphicsObject
{
	Q_OBJECT

public:
	explicit VoiceItem(const void* id, QGraphicsItem* parent = nullptr);

	QRectF boundingRect() const override;
	void paint(QPainter*, const QStyleOptionGraphicsItem*, QWidget*) override;
	qreal requiredWidth() const;
	void setHighlighted(bool);
	void setTrackCount(size_t);
	void setVoiceIndex(size_t);
	void setVoiceName(const QString& name);
	void setWidth(qreal);
	size_t trackCount() const noexcept { return _trackCount; }
	const void* voiceId() const noexcept { return _voiceId; }
	size_t voiceIndex() const noexcept { return _index; }

signals:
	void voiceActionRequested(const void* voiceId);
	void voiceMenuRequested(const void* voiceId, const QPoint& pos);
	void voiceSelected(const void* voiceId);

private:
	void contextMenuEvent(QGraphicsSceneContextMenuEvent*) override;
	void mouseDoubleClickEvent(QGraphicsSceneMouseEvent*) override;
	void mousePressEvent(QGraphicsSceneMouseEvent*) override;

private:
	const void* const _voiceId;
	size_t _index = 0;
	qreal _width = 0;
	size_t _trackCount = 0;
	QStaticText _name;
	bool _highlighted = false;
};
