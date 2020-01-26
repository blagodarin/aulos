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

#include <QGraphicsObject>

class TrackItem final : public QGraphicsObject
{
	Q_OBJECT

public:
	TrackItem(const void* id, QGraphicsItem* parent = nullptr);

	QRectF boundingRect() const override;
	void setFirstVoiceTrack(bool);
	void setTrackIndex(size_t);
	void setTrackLength(size_t);
	const void* trackId() const noexcept { return _trackId; }
	size_t trackIndex() const noexcept { return _index; }

signals:
	void trackMenuRequested(const void* trackId, size_t offset, const QPoint& pos);

private:
	void contextMenuEvent(QGraphicsSceneContextMenuEvent*) override;
	void paint(QPainter*, const QStyleOptionGraphicsItem*, QWidget*) override;

private:
	const void* const _trackId;
	size_t _length = 0;
	size_t _index = 0;
	bool _first = false;
};
