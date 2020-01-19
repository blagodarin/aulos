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

#include <QGraphicsObject>

namespace aulos
{
	struct CompositionData;
	struct SequenceData;
}

class TrackItem : public QGraphicsObject
{
	Q_OBJECT

public:
	TrackItem(const std::shared_ptr<const aulos::CompositionData>&, size_t trackIndex, QGraphicsItem* parent = nullptr);

	QRectF boundingRect() const override { return _rect; }
	void paint(QPainter*, const QStyleOptionGraphicsItem*, QWidget*) override;
	void setTrackLength(size_t length);
	size_t trackIndex() const noexcept { return _trackIndex; }

signals:
	void insertRequested(size_t trackIndex, size_t offset, const std::shared_ptr<const aulos::SequenceData>&);
	void newSequenceRequested(size_t trackIndex, size_t offset);

private:
	void contextMenuEvent(QGraphicsSceneContextMenuEvent*) override;

private:
	const std::shared_ptr<const aulos::CompositionData> _composition;
	const size_t _trackIndex;
	size_t _length = 0;
	QRectF _rect;
};
