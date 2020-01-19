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
#include "timeline_item.hpp"
#include "track_item.hpp"
#include "utils.hpp"
#include "voice_item.hpp"

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
	VoiceItem* _header = nullptr;
	TrackItem* _background = nullptr;
	std::map<size_t, FragmentItem*> _fragments;
};

CompositionScene::CompositionScene()
	: _timeline{ std::make_unique<TimelineItem>() }
{
	setBackgroundBrush(kBackgroundColor);
}

void CompositionScene::insertFragment(size_t trackIndex, size_t offset, const std::shared_ptr<const aulos::SequenceData>& sequence)
{
	const auto newItem = addFragmentItem(trackIndex, offset, sequence);
	const auto minCompositionLength = offset + newItem->fragmentLength() + kExtraLength;
	if (minCompositionLength <= _timeline->compositionLength())
		return;
	auto compositionRect = sceneRect();
	compositionRect.setRight(minCompositionLength * kStepWidth);
	setSceneRect(compositionRect);
	_timeline->setCompositionLength(minCompositionLength);
	for (const auto& track : _tracks)
		track->_background->setTrackLength(minCompositionLength);
}

void CompositionScene::removeFragment(size_t trackIndex, size_t offset)
{
	auto& track = *_tracks[trackIndex];
	const auto i = track._fragments.find(offset);
	assert(i != track._fragments.end());
	removeItem(i->second);
	update(i->second->boundingRect()); // The view doesn't update properly if it's inside a scroll area.
	i->second->deleteLater();
	track._fragments.erase(i);
}

void CompositionScene::reset(const std::shared_ptr<const aulos::CompositionData>& composition)
{
	removeItem(_timeline.get());
	_tracks.clear();
	_cursorItem = nullptr;
	clear();

	_composition = composition;
	if (!_composition || _composition->_tracks.empty())
		return;

	qreal trackHeaderWidth = 0;
	size_t compositionLength = 0;
	for (size_t i = 0; i < _composition->_tracks.size(); ++i)
	{
		const auto& track = _tracks.emplace_back(std::make_unique<Track>());

		const auto header = new VoiceItem{ _composition, i };
		addItem(header);
		track->_header = header;
		trackHeaderWidth = std::max(trackHeaderWidth, header->requiredWidth());

		const auto item = new TrackItem{ _composition, i };
		addItem(item);
		connect(item, &TrackItem::insertRequested, this, &CompositionScene::insertFragmentRequested);
		connect(item, &TrackItem::newSequenceRequested, this, &CompositionScene::newSequenceRequested);
		track->_background = item;

		if (i > 0)
		{
			header->stackBefore(_tracks[i - 1]->_header);
			item->stackBefore(_tracks[i - 1]->_background);
		}

		for (const auto& fragment : _composition->_tracks[i]->_fragments)
		{
			const auto fragmentItem = addFragmentItem(i, fragment.first, fragment.second);
			compositionLength = std::max(compositionLength, fragmentItem->fragmentOffset() + fragmentItem->fragmentLength());
		}
	}
	compositionLength += kExtraLength;

	_timeline->setCompositionSpeed(_composition->_speed);
	_timeline->setCompositionLength(compositionLength);
	addItem(_timeline.get());
	for (const auto& track : _tracks)
	{
		track->_header->setWidth(trackHeaderWidth);
		track->_background->setTrackLength(compositionLength);
	}

	const auto bottomRight = _tracks.back()->_background->boundingRect().bottomRight();
	setSceneRect({ { -trackHeaderWidth, -kTimelineHeight }, bottomRight });

	_cursorItem = new QGraphicsLineItem{ 0.0, -kTimelineHeight, 0.0, bottomRight.y() };
	_cursorItem->setPen(kCursorColor);
	_cursorItem->setVisible(false);
	_cursorItem->setZValue(1.0);
	addItem(_cursorItem);
}

void CompositionScene::setCurrentStep(double step)
{
	if (_cursorItem)
	{
		const auto x = step * kStepWidth;
		const auto isVisible = x >= 0.0 && x < sceneRect().right();
		_cursorItem->setVisible(isVisible);
		if (isVisible)
			_cursorItem->setTransform(QTransform{ 1.0, 0.0, 0.0, 1.0, step * kStepWidth, 0.0 });
	}
}

void CompositionScene::setSpeed(unsigned speed)
{
	_timeline->setCompositionSpeed(speed);
}

void CompositionScene::onEditRequested(size_t trackIndex, size_t offset, const std::shared_ptr<const aulos::SequenceData>&)
{
	(void)trackIndex, offset;
}

FragmentItem* CompositionScene::addFragmentItem(size_t trackIndex, size_t offset, const std::shared_ptr<const aulos::SequenceData>& sequence)
{
	auto& track = *_tracks[trackIndex];
	const auto item = new FragmentItem{ trackIndex, offset, sequence, track._background };
	connect(item, &FragmentItem::editRequested, this, &CompositionScene::onEditRequested);
	connect(item, &FragmentItem::removeRequested, this, &CompositionScene::removeFragmentRequested);
	const auto i = track._fragments.emplace(offset, item).first;
	if (i != track._fragments.begin())
		std::prev(i)->second->stackBefore(i->second);
	if (const auto next = std::next(i); next != track._fragments.end())
		i->second->stackBefore(next->second);
	return item;
}
