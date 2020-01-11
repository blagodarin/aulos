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

#include "fragment_item.hpp"
#include "track_item.hpp"

#include <aulos/data.hpp>

#include <array>
#include <numeric>
#include <vector>

#include <QGraphicsRectItem>

namespace
{
	const QColor kBackgroundColor{ "#444" };
	const QColor kCursorColor{ "#000" };
	constexpr size_t kExtraLength = 1;
}

struct CompositionScene::Track
{
	TrackItem* _background = nullptr;
	std::vector<size_t> _sequenceLengths;
	std::map<size_t, FragmentItem*> _fragments;
};

CompositionScene::CompositionScene()
{
	setBackgroundBrush(kBackgroundColor);
}

void CompositionScene::reset(const aulos::CompositionData* composition)
{
	_compositionLength = 0;
	_tracks.clear();
	_cursorItem = nullptr;
	clear();

	if (!composition || composition->_tracks.empty())
		return;

	for (size_t trackIndex = 0; trackIndex < composition->_tracks.size(); ++trackIndex)
	{
		const auto& trackData = composition->_tracks[trackIndex];
		auto& track = *_tracks.emplace_back(std::make_unique<Track>());

		track._sequenceLengths.reserve(trackData._sequences.size());
		for (const auto& sequenceData : trackData._sequences)
		{
			size_t sequenceLength = 0;
			for (const auto& soundData : sequenceData)
				sequenceLength += soundData._pause;
			track._sequenceLengths.emplace_back(sequenceLength);
		}

		size_t offset = 0;
		for (const auto& fragment : trackData._fragments)
		{
			offset += fragment._delay;
			const auto item = addFragmentItem(trackIndex, offset, fragment._sequence);
			_compositionLength = std::max(_compositionLength, offset + item->fragmentLength());
		}
	}

	_compositionLength += kExtraLength;

	for (size_t i = 0; i < composition->_tracks.size(); ++i)
	{
		const auto item = new TrackItem{ i, _compositionLength };
		item->setZValue(-1.0);
		addItem(item);
		connect(item, &TrackItem::insertRequested, this, &CompositionScene::onInsertRequested);
		_tracks[i]->_background = item;
	}

	const auto bottomRight = _tracks.back()->_background->boundingRect().bottomRight();
	setSceneRect(0.0, 0.0, bottomRight.x(), bottomRight.y());

	_cursorItem = new QGraphicsLineItem{ 0.0, 0.0, 0.0, bottomRight.y() };
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

void CompositionScene::onEditRequested(size_t trackIndex, size_t offset, size_t sequenceIndex)
{
	(void)trackIndex, offset, sequenceIndex;
}

void CompositionScene::onInsertRequested(size_t trackIndex, size_t offset)
{
	const auto newItem = addFragmentItem(trackIndex, offset, 0);
	const auto minCompositionLength = offset + newItem->fragmentLength() + kExtraLength;
	if (minCompositionLength <= _compositionLength)
		return;
	_compositionLength = minCompositionLength;
	auto compositionRect = sceneRect();
	compositionRect.setWidth(_compositionLength * kScaleX);
	setSceneRect(compositionRect);
	for (const auto& track : _tracks)
		track->_background->setTrackLength(_compositionLength);
}

void CompositionScene::onRemoveRequested(size_t trackIndex, size_t offset)
{
	auto& track = *_tracks[trackIndex];
	const auto i = track._fragments.find(offset);
	assert(i != track._fragments.end());
	removeItem(i->second);
	i->second->deleteLater();
	track._fragments.erase(i);
}

FragmentItem* CompositionScene::addFragmentItem(size_t trackIndex, size_t offset, size_t sequenceIndex)
{
	auto& track = *_tracks[trackIndex];
	const auto item = new FragmentItem{ trackIndex, offset, sequenceIndex, track._sequenceLengths[sequenceIndex] };
	addItem(item);
	connect(item, &FragmentItem::editRequested, this, &CompositionScene::onEditRequested);
	connect(item, &FragmentItem::removeRequested, this, &CompositionScene::onRemoveRequested);
	const auto i = track._fragments.emplace(offset, item).first;
	if (i != track._fragments.begin())
		std::prev(i)->second->stackBefore(i->second);
	if (const auto next = std::next(i); next != track._fragments.end())
		i->second->stackBefore(next->second);
	return item;
}
