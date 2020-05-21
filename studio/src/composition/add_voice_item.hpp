// This file is part of the Aulos toolkit.
// Copyright (C) Sergei Blagodarin.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "../button_item.hpp"

class AddVoiceItem final : public ButtonItem
{
	Q_OBJECT

public:
	explicit AddVoiceItem(QGraphicsItem* parent = nullptr);

	QRectF boundingRect() const override;
	void paint(QPainter*, const QStyleOptionGraphicsItem*, QWidget*) override;
	void setIndex(size_t);
	void setWidth(qreal);

private:
	size_t _index = 0;
	qreal _width = 0;
};
