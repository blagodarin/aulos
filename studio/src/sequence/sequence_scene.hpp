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

#include <memory>

#include <QGraphicsScene>

class ElusiveItem;
class PianorollItem;
class SoundItem;

class SequenceScene final : public QGraphicsScene
{
	Q_OBJECT

public:
	explicit SequenceScene(QObject* parent = nullptr);
	~SequenceScene() override;

	void insertSound(size_t offset, aulos::Note);
	void removeSound(size_t offset, aulos::Note);
	qreal setSequence(const aulos::SequenceData&, const QSize& viewSize);

signals:
	void insertingSound(size_t offset, aulos::Note);
	void noteActivated(aulos::Note);
	void removingSound(size_t offset, aulos::Note);

private:
	void insertNewSound(size_t offset, aulos::Note);
	void removeSoundItems();
	void setPianorollLength(size_t steps);

private:
	std::unique_ptr<PianorollItem> _pianorollItem;
	ElusiveItem* _rightBoundItem = nullptr;
	std::multimap<size_t, std::unique_ptr<SoundItem>> _soundItems;
};
