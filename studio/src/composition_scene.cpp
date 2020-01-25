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

#include "add_voice_item.hpp"
#include "fragment_item.hpp"
#include "timeline_item.hpp"
#include "track_item.hpp"
#include "utils.hpp"
#include "voice_item.hpp"

#include <aulos/data.hpp>

#include <cassert>
#include <numeric>
#include <vector>

#include <QGraphicsRectItem>

namespace
{
	const QColor kBackgroundColor{ "#444" };
	const QColor kCursorColor{ "#000" };
	constexpr size_t kExtraLength = 1;

	QLineF makeCursorLine(size_t height)
	{
		return { 0, -kTimelineHeight, 0, height * kTrackHeight };
	}
}

struct CompositionScene::Track
{
	TrackItem* _background = nullptr;
	std::map<size_t, FragmentItem*> _fragments;
};

CompositionScene::CompositionScene()
	: _timelineItem{ std::make_unique<TimelineItem>() }
	, _cursorItem{ std::make_unique<QGraphicsLineItem>(::makeCursorLine(0)) }
	, _addVoiceItem{ std::make_unique<AddVoiceItem>() }
	, _voiceColumnWidth{ kMinVoiceItemWidth }
{
	setBackgroundBrush(kBackgroundColor);
	_timelineItem->setZValue(0.5);
	connect(_timelineItem.get(), &TimelineItem::lengthRequested, this, &CompositionScene::setCompositionLength);
	_addVoiceItem->setZValue(0.5);
	_addVoiceItem->setWidth(_voiceColumnWidth);
	connect(_addVoiceItem.get(), &ButtonItem::clicked, this, &CompositionScene::newVoiceRequested);
	_cursorItem->setPen(kCursorColor);
	_cursorItem->setVisible(false);
	_cursorItem->setZValue(1.0);
}

CompositionScene::~CompositionScene() = default;

void CompositionScene::appendPart(const std::shared_ptr<aulos::PartData>& partData)
{
	assert(partData->_tracks.size() == 1);
	assert(partData->_tracks.front()->_fragments.empty());

	const auto voiceIndex = _voices.size();
	const auto voiceItem = _voices.emplace_back(std::make_unique<VoiceItem>(partData->_voice)).get();
	voiceItem->setIndex(voiceIndex);
	voiceItem->setPos(0, _tracks.size() * kTrackHeight);
	voiceItem->setTrackCount(partData->_tracks.size());
	voiceItem->setWidth(_voices.size() > 1 ? _voices.front()->boundingRect().width() : kMinVoiceItemWidth);

	const auto& track = _tracks.emplace_back(std::make_unique<Track>());

	track->_background = new TrackItem{ partData->_tracks.front(), voiceItem };
	track->_background->setTrackLength(_timelineItem->compositionLength());
	track->_background->setTrackIndices(_tracks.size() - 1, 0);
	connect(track->_background, &TrackItem::insertRequested, this, &CompositionScene::insertFragmentRequested);
	connect(track->_background, &TrackItem::newSequenceRequested, this, &CompositionScene::newSequenceRequested);

	bool shouldUpdateVoiceColumnWidth = false;
	if (const auto requiredWidth = voiceItem->requiredWidth(); requiredWidth > _voiceColumnWidth)
	{
		_voiceColumnWidth = requiredWidth;
		shouldUpdateVoiceColumnWidth = true;
	}
	else
		voiceItem->setWidth(_voiceColumnWidth);
	updateSceneRect(_timelineItem->compositionLength());
	if (shouldUpdateVoiceColumnWidth)
		setVoiceColumnWidth(_voiceColumnWidth);

	addItem(voiceItem);
	if (_voices.size() > 1)
		_voices[_voices.size() - 2]->stackBefore(voiceItem);

	_addVoiceItem->setIndex(_voices.size());
	_addVoiceItem->setPos(0, _tracks.size() * kTrackHeight);

	_cursorItem->setLine(::makeCursorLine(_tracks.size()));
}

void CompositionScene::insertFragment(const aulos::TrackData* trackData, size_t offset, const std::shared_ptr<aulos::SequenceData>& sequence)
{
	const auto track = std::find_if(_tracks.begin(), _tracks.end(), [trackData](const auto& trackPtr) { return trackPtr->_background->trackData().get() == trackData; });
	assert(track != _tracks.end());
	const auto newItem = addFragmentItem(**track, offset, sequence);
	const auto minCompositionLength = offset + newItem->fragmentLength() + kExtraLength;
	if (minCompositionLength > _timelineItem->compositionLength())
		setCompositionLength(minCompositionLength);
}

void CompositionScene::removeFragment(const aulos::TrackData* trackData, size_t offset)
{
	const auto track = std::find_if(_tracks.begin(), _tracks.end(), [trackData](const auto& trackPtr) { return trackPtr->_background->trackData().get() == trackData; });
	assert(track != _tracks.end());
	const auto fragment = (*track)->_fragments.find(offset);
	assert(fragment != (*track)->_fragments.end());
	removeItem(fragment->second);
	fragment->second->deleteLater();
	(*track)->_fragments.erase(fragment);
}

void CompositionScene::reset(const std::shared_ptr<aulos::CompositionData>& composition)
{
	removeItem(_timelineItem.get());
	_voices.clear();
	_tracks.clear();
	removeItem(_addVoiceItem.get());
	removeItem(_cursorItem.get());
	clear();

	_composition = composition;
	if (!_composition || _composition->_parts.empty())
		return;

	size_t compositionLength = 0;
	_voices.reserve(_composition->_parts.size());
	for (const auto& partData : _composition->_parts)
	{
		assert(!partData->_tracks.empty());

		const auto voiceIndex = _voices.size();
		const auto voiceItem = _voices.emplace_back(std::make_unique<VoiceItem>(partData->_voice)).get();
		voiceItem->setIndex(voiceIndex);
		voiceItem->setPos(0, _tracks.size() * kTrackHeight);
		voiceItem->setTrackCount(partData->_tracks.size());

		const auto trackIndexBase = _tracks.size();
		for (auto i = partData->_tracks.crbegin(); i != partData->_tracks.crend(); ++i)
		{
			const auto trackOffset = std::prev(i.base()) - partData->_tracks.cbegin();
			const auto& track = _tracks.emplace_back(std::make_unique<Track>());

			track->_background = new TrackItem{ *i, voiceItem };
			track->_background->setPos(0, trackOffset * kTrackHeight);
			track->_background->setTrackIndices(trackIndexBase + trackOffset, trackOffset);
			connect(track->_background, &TrackItem::insertRequested, this, &CompositionScene::insertFragmentRequested);
			connect(track->_background, &TrackItem::newSequenceRequested, this, &CompositionScene::newSequenceRequested);

			for (const auto& fragment : (*i)->_fragments)
			{
				const auto fragmentItem = addFragmentItem(*track, fragment.first, fragment.second);
				compositionLength = std::max(compositionLength, fragmentItem->fragmentOffset() + fragmentItem->fragmentLength());
			}
		}
	}
	compositionLength += kExtraLength;

	_timelineItem->setCompositionSpeed(_composition->_speed);
	_timelineItem->setCompositionLength(compositionLength);
	for (const auto& track : _tracks)
		track->_background->setTrackLength(compositionLength);
	_addVoiceItem->setIndex(_voices.size());
	_addVoiceItem->setPos(0, _tracks.size() * kTrackHeight);
	_cursorItem->setLine(::makeCursorLine(_tracks.size()));
	_cursorItem->setVisible(false);

	setVoiceColumnWidth(requiredVoiceColumnWidth());
	updateSceneRect(compositionLength);
	addItem(_timelineItem.get());
	for (auto i = _voices.crbegin(); i != _voices.crend(); ++i)
		addItem(i->get());
	addItem(_addVoiceItem.get());
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
	_timelineItem->setCompositionSpeed(speed);
}

void CompositionScene::setCompositionLength(size_t length)
{
	updateSceneRect(length);
	_timelineItem->setCompositionLength(length);
	for (const auto& track : _tracks)
		track->_background->setTrackLength(length);
}

FragmentItem* CompositionScene::addFragmentItem(Track& track, size_t offset, const std::shared_ptr<aulos::SequenceData>& sequence)
{
	const auto item = new FragmentItem{ track._background, offset, sequence };
	item->setPos(offset * kStepWidth, 0);
	connect(item, &FragmentItem::removeRequested, this, &CompositionScene::removeFragmentRequested);
	const auto i = track._fragments.emplace(offset, item).first;
	if (i != track._fragments.begin())
		std::prev(i)->second->stackBefore(i->second);
	if (const auto next = std::next(i); next != track._fragments.end())
		i->second->stackBefore(next->second);
	return item;
}

qreal CompositionScene::requiredVoiceColumnWidth() const
{
	qreal width = kMinVoiceItemWidth;
	for (const auto& voice : _voices)
		width = std::max(width, voice->requiredWidth());
	return width;
}

void CompositionScene::setVoiceColumnWidth(qreal width)
{
	_voiceColumnWidth = width;
	for (const auto& voice : _voices)
		voice->setWidth(width);
	_addVoiceItem->setWidth(width);
}

void CompositionScene::updateSceneRect(size_t compositionLength)
{
	setSceneRect({ { -_voiceColumnWidth, -kTimelineHeight }, QPointF{ compositionLength * kStepWidth + kAddTimeItemWidth + kAddTimeExtraWidth, _tracks.size() * kTrackHeight + kAddVoiceItemHeight } });
}
