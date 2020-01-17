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

#include <memory>

#include <QGraphicsItem>

class TimelineItem : public QGraphicsItem
{
public:
	TimelineItem(size_t speed, size_t length, QGraphicsItem* parent = nullptr);

	QRectF boundingRect() const override { return _rect; }
	void paint(QPainter*, const QStyleOptionGraphicsItem*, QWidget*) override;
	void setCompositionSpeed(size_t speed);
	void setTrackLength(size_t length);
	size_t trackLength() const noexcept { return _length; }

private:
	size_t _speed;
	size_t _length;
	QRectF _rect;
};
