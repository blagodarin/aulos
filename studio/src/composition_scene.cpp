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

#include "composition_scene.hpp"

#include "composition_item.hpp"

#include <aulos.hpp>

#include <array>
#include <numeric>
#include <vector>

#include <QGraphicsRectItem>

namespace
{
	const QColor kBackgroundColor{ "#444" };
	const std::array<QColor, 2> kTrackBackgroundColor{ "#999", "#888" };
	const QColor kCursorColor{ "#000" };
}

CompositionScene::CompositionScene()
{
	setBackgroundBrush(kBackgroundColor);
}

void CompositionScene::reset(const aulos::Composition* composition)
{
	_cursorItem = nullptr;
	clear();

	if (!composition)
		return;

	const auto trackCount = composition->trackCount();
	if (!trackCount)
		return;

	double maxRight = 0.0;
	for (size_t trackIndex = 0; trackIndex < trackCount; ++trackIndex)
	{
		std::unordered_map<size_t, size_t> sequenceLengths;
		const auto sequenceCount = composition->sequenceCount(trackIndex);
		sequenceLengths.reserve(sequenceCount);
		for (size_t sequenceIndex = 0; sequenceIndex < sequenceCount; ++sequenceIndex)
		{
			const auto sequence = composition->sequence(trackIndex, sequenceIndex);
			size_t sequenceLength = 0;
			for (size_t i = 0; i < sequence._size; ++i)
				sequenceLength += sequence._data[i]._pause;
			sequenceLengths.emplace(sequenceIndex, sequenceLength);
		}

		std::vector<aulos::Fragment> fragments;
		const auto fragmentCount = composition->fragmentCount(trackIndex);
		fragments.reserve(fragmentCount);
		for (size_t i = 0; i < fragmentCount; ++i)
			fragments.emplace_back(composition->fragment(trackIndex, i));

		size_t offset = 0;
		for (const auto& fragment : fragments)
		{
			offset += fragment._delay;
			const auto duration = sequenceLengths[fragment._sequence];
			const auto item = new CompositionItem{ trackIndex, offset, fragment._sequence, duration };
			addItem(item);
			maxRight = std::max(maxRight, item->boundingRect().right());
		}
	}

	maxRight += kScaleX; // So the end is clearly visible.

	for (size_t i = 0; i < trackCount; ++i)
	{
		const auto item = new QGraphicsRectItem{ 0.0, i * kScaleY, maxRight, kScaleY };
		item->setBrush(kTrackBackgroundColor[i % kTrackBackgroundColor.size()]);
		item->setPen(QColor{ Qt::transparent });
		item->setZValue(-1.0);
		addItem(item);
	}

	const auto totalHeight = trackCount * kScaleY;
	setSceneRect(0.0, 0.0, maxRight, totalHeight);

	_cursorItem = new QGraphicsLineItem{ 0.0, 0.0, 0.0, totalHeight };
	_cursorItem->setPen(kCursorColor);
	_cursorItem->setVisible(false);
	_cursorItem->setZValue(1.0);
	addItem(_cursorItem);
}

void CompositionScene::setCurrentStep(double step)
{
	if (_cursorItem)
	{
		const auto x = step * kScaleX;
		const auto isVisible = x >= 0.0 && x < sceneRect().right();
		_cursorItem->setVisible(isVisible);
		if (isVisible)
			_cursorItem->setTransform(QTransform{ 1.0, 0.0, 0.0, 1.0, step * kScaleX, 0.0 });
	}
}
