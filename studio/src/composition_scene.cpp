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

#include <aulos.hpp>

#include <array>
#include <numeric>
#include <vector>

#include <QGraphicsRectItem>

namespace
{
	constexpr auto kScaleX = 10.0;
	constexpr auto kScaleY = 40.0;

	const std::array<QColor, 2> kTrackBackgroundColor{ "#0fe", "#0ef" };
	const std::array<QColor, 2> kFragmentBackgroundColor{ "#0ec", "#0ce" };
	const std::array<QColor, 2> kFragmentFrameColor{ "#080", "#008" };

	class FragmentItem : public QGraphicsRectItem
	{
	public:
		FragmentItem(size_t x, size_t y, size_t duration)
			: QGraphicsRectItem{ x * kScaleX, y * kScaleY, duration * kScaleX, kScaleY } {}
	};
}

CompositionScene::CompositionScene()
{
	setBackgroundBrush(Qt::darkGray);
}

void CompositionScene::reset(const aulos::Composition* composition)
{
	clear();

	if (!composition)
		return;

	const auto trackCount = composition->trackCount();
	if (!trackCount)
		return;

	std::vector<size_t> sequenceOffsets;
	sequenceOffsets.reserve(trackCount + 1);
	sequenceOffsets.emplace_back(0);
	for (size_t i = 0; i < trackCount; ++i)
		sequenceOffsets.emplace_back(sequenceOffsets[i] + composition->sequenceCount(i));
	if (!sequenceOffsets.back())
		return;

	QHash<QPair<size_t, size_t>, size_t> sequenceLengths;
	for (size_t t = 0; t < trackCount; ++t)
	{
		for (size_t s = 0; s < composition->sequenceCount(t); ++s)
		{
			const auto sequence = composition->sequence(t, s);
			size_t sequenceLength = 1;
			for (size_t i = 0; i < sequence._size; ++i)
				sequenceLength += sequence._data[i]._delay;
			sequenceLengths.insert(qMakePair(t, s), sequenceLength);
		}
	}

	const auto fragmentCount = composition->fragmentCount();
	std::vector<aulos::Fragment> fragments;
	fragments.reserve(fragmentCount);
	for (size_t i = 0; i < fragmentCount; ++i)
		fragments.emplace_back(composition->fragment(i));

	size_t offset = 0;
	double maxRight = 0.0;
	for (const auto& fragment : fragments)
	{
		offset += fragment._delay;
		const auto duration = sequenceLengths.value(qMakePair(fragment._track, fragment._sequence));
		const auto item = new FragmentItem{ offset, sequenceOffsets[fragment._track] + fragment._sequence, duration };
		item->setBrush(kFragmentBackgroundColor[fragment._track & 1]);
		item->setPen(kFragmentFrameColor[fragment._track & 1]);
		addItem(item);
		maxRight = std::max(maxRight, item->rect().right());
	}

	maxRight += kScaleX; // So the end is clearly visible.

	for (size_t i = 0; i < trackCount; ++i)
	{
		const auto item = new QGraphicsRectItem{ 0.0, sequenceOffsets[i] * kScaleY, maxRight, (sequenceOffsets[i + 1] - sequenceOffsets[i]) * kScaleY };
		item->setBrush(kTrackBackgroundColor[i & 1]);
		item->setPen(QColor{ Qt::transparent });
		item->setZValue(-1.0);
		addItem(item);
	}

	setSceneRect(0.0, 0.0, maxRight, sequenceOffsets.back() * kScaleY);
}
