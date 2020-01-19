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
	, _cursorItem{ std::make_unique<QGraphicsLineItem>(0, -kTimelineHeight, 0, 0) }
{
	setBackgroundBrush(kBackgroundColor);
	_cursorItem->setPen(kCursorColor);
	_cursorItem->setVisible(false);
	_cursorItem->setZValue(1.0);
}

void CompositionScene::insertFragment(size_t trackIndex, size_t offset, const std::shared_ptr<const aulos::SequenceData>& sequence)
{
	const auto newItem = addFragmentItem(*_tracks[trackIndex], offset, sequence);
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
	removeItem(_cursorItem.get());
	clear();

	_composition = composition;
	if (!_composition || _composition->_tracks.empty())
		return;

	qreal trackHeaderWidth = 0;
	size_t compositionLength = 0;
	for (const auto& trackData : _composition->_tracks)
	{
		const auto trackIndex = _tracks.size();
		const auto& track = _tracks.emplace_back(std::make_unique<Track>());

		track->_header = new VoiceItem{ _composition, trackIndex };
		trackHeaderWidth = std::max(trackHeaderWidth, track->_header->requiredWidth());

		track->_background = new TrackItem{ _composition, trackIndex };
		connect(track->_background, &TrackItem::insertRequested, this, &CompositionScene::insertFragmentRequested);
		connect(track->_background, &TrackItem::newSequenceRequested, this, &CompositionScene::newSequenceRequested);

		for (const auto& fragment : trackData->_fragments)
		{
			const auto fragmentItem = addFragmentItem(*track, fragment.first, fragment.second);
			compositionLength = std::max(compositionLength, fragmentItem->fragmentOffset() + fragmentItem->fragmentLength());
		}
	}
	compositionLength += kExtraLength;

	_timeline->setCompositionSpeed(_composition->_speed);
	_timeline->setCompositionLength(compositionLength);
	for (const auto& track : _tracks)
	{
		track->_header->setWidth(trackHeaderWidth);
		track->_background->setTrackLength(compositionLength);
	}

	const auto bottomRight = _tracks.back()->_background->boundingRect().bottomRight();
	setSceneRect({ { -trackHeaderWidth, -kTimelineHeight }, bottomRight });

	auto cursorLine = _cursorItem->line();
	cursorLine.setP2({ 0, bottomRight.y() });
	_cursorItem->setLine(cursorLine);
	_cursorItem->setVisible(false);

	addItem(_timeline.get());
	for (auto i = _tracks.crbegin(); i != _tracks.crend(); ++i)
	{
		addItem((*i)->_header);
		addItem((*i)->_background);
	}
	addItem(_cursorItem.get());
}

void CompositionScene::setCurrentStep(double step)
{
	if (_composition)
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
	if (_timeline->setCompositionSpeed(speed))
		_timeline->update();
}

void CompositionScene::onEditRequested(size_t trackIndex, size_t offset, const std::shared_ptr<const aulos::SequenceData>&)
{
	(void)trackIndex, offset;
}

FragmentItem* CompositionScene::addFragmentItem(Track& track, size_t offset, const std::shared_ptr<const aulos::SequenceData>& sequence)
{
	const auto item = new FragmentItem{ track._background, offset, sequence };
	connect(item, &FragmentItem::editRequested, this, &CompositionScene::onEditRequested);
	connect(item, &FragmentItem::removeRequested, this, &CompositionScene::removeFragmentRequested);
	const auto i = track._fragments.emplace(offset, item).first;
	if (i != track._fragments.begin())
		std::prev(i)->second->stackBefore(i->second);
	if (const auto next = std::next(i); next != track._fragments.end())
		i->second->stackBefore(next->second);
	return item;
}
