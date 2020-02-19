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
