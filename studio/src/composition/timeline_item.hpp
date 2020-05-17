//
// This file is part of the Aulos toolkit.
// Copyright (C) 2020 Sergei Blagodarin.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//

#pragma once

#include <QGraphicsItem>

class TimelineItem final : public QGraphicsItem
{
public:
	explicit TimelineItem(QGraphicsItem* parent = nullptr);

	QRectF boundingRect() const override;
	void paint(QPainter*, const QStyleOptionGraphicsItem*, QWidget*) override;
	void setCompositionLength(size_t length);
	void setCompositionOffset(size_t offset);
	void setCompositionSpeed(unsigned speed);
	size_t compositionLength() const noexcept { return _length; }
	size_t compositionOffset() const noexcept { return _offset; }
	unsigned compositionSpeed() const noexcept { return _speed; }

private:
	void mousePressEvent(QGraphicsSceneMouseEvent*) override;

private:
	unsigned _speed = 1;
	size_t _length = 0;
	size_t _offset = 0;
	QPolygonF _offsetMark;
};
