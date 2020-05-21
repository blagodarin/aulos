// This file is part of the Aulos toolkit.
// Copyright (C) Sergei Blagodarin.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <QGraphicsObject>

class ButtonItem : public QGraphicsObject
{
	Q_OBJECT

public:
	enum class Mode
	{
		Click,
		Press,
	};

	explicit ButtonItem(Mode mode = Mode::Click, QGraphicsItem* parent = nullptr);

signals:
	void activated();

protected:
	bool isHovered() const noexcept { return _hovered; }
	bool isPressed() const noexcept { return _pressed; }

private:
	void hoverEnterEvent(QGraphicsSceneHoverEvent*) override;
	void hoverLeaveEvent(QGraphicsSceneHoverEvent*) override;
	void mousePressEvent(QGraphicsSceneMouseEvent*) override;
	void mouseReleaseEvent(QGraphicsSceneMouseEvent*) override;

private:
	const Mode _mode;
	bool _hovered = false;
	bool _pressed = false;
};
