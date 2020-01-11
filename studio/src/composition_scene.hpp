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

#include <QGraphicsScene>

class QGraphicsLineItem;

namespace aulos
{
	class Composition;
}

constexpr auto kScaleX = 10.0;
constexpr auto kScaleY = 40.0;

class FragmentItem;

class CompositionScene : public QGraphicsScene
{
	Q_OBJECT

public:
	CompositionScene();

	void reset(const aulos::Composition*);
	void setCurrentStep(double step);

private slots:
	void onEditRequested(size_t trackIndex, size_t offset, size_t sequenceIndex);
	void onInsertRequested(size_t trackIndex, size_t offset);
	void onRemoveRequested(size_t trackIndex, size_t offset);

private:
	FragmentItem* addFragmentItem(size_t trackIndex, size_t offset, size_t sequenceIndex);

private:
	struct Track;
	size_t _compositionLength = 0;
	std::vector<std::unique_ptr<Track>> _tracks;
	QGraphicsLineItem* _cursorItem = nullptr;
};
