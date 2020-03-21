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
