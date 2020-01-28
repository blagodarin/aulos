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

	auto findVoice(const std::vector<std::unique_ptr<VoiceItem>>& voices, const void* id)
	{
		size_t offset = 0;
		for (auto i = voices.begin(); i != voices.end(); ++i)
		{
			if ((*i)->voiceId() == id)
				return std::pair{ i, offset };
			offset += (*i)->trackCount();
		}
		return std::pair{ voices.end(), offset };
	}

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

void CompositionScene::addTrack(const void* voiceId, const void* trackId)
{
	const auto [voice, voiceOffset] = ::findVoice(_voices, voiceId);
	assert(voice != _voices.end());
	const auto trackOffset = (*voice)->trackCount();
	const auto trackIndex = voiceOffset + trackOffset;
	const auto track = _tracks.emplace(_tracks.begin() + trackIndex, std::make_unique<Track>());
	(*track)->_background = addTrackItem(voice->get(), trackId);
	(*track)->_background->setFirstTrack(trackOffset == 0);
	(*track)->_background->setPos(0, trackOffset * kTrackHeight);
	(*track)->_background->setTrackIndex(trackIndex);
	(*track)->_background->setTrackLength(_timelineItem->compositionLength());
	if (trackOffset > 0)
		(*track)->_background->stackBefore((*std::prev(track))->_background);
	(*voice)->setTrackCount(trackOffset + 1);
	std::for_each(voice + 1, _voices.cend(), [index = trackIndex + 1](const auto& voiceItem) mutable {
		voiceItem->setPos(0, index * kTrackHeight);
		index += voiceItem->trackCount();
	});
	std::for_each(track + 1, _tracks.end(), [index = trackIndex + 1](const auto& trackItem) mutable {
		trackItem->_background->setTrackIndex(index);
		++index;
	});
	_addVoiceItem->setPos(0, _tracks.size() * kTrackHeight);
	_cursorItem->setLine(::makeCursorLine(_tracks.size()));
	updateSceneRect(_timelineItem->compositionLength());
}

void CompositionScene::appendPart(const std::shared_ptr<aulos::PartData>& partData)
{
	assert(partData->_tracks.size() == 1);
	assert(partData->_tracks.front()->_fragments.empty());

	const auto voiceItem = addVoiceItem(partData->_voice.get(), QString::fromStdString(partData->_voice->_name), partData->_tracks.size());

	const auto& track = _tracks.emplace_back(std::make_unique<Track>());
	track->_background = addTrackItem(voiceItem, partData->_tracks.front().get());
	track->_background->setFirstTrack(true);
	track->_background->setTrackIndex(_tracks.size() - 1);
	track->_background->setTrackLength(_timelineItem->compositionLength());

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
		voiceItem->stackBefore(_voices[_voices.size() - 2].get());

	_addVoiceItem->setIndex(_voices.size());
	_addVoiceItem->setPos(0, _tracks.size() * kTrackHeight);

	_cursorItem->setLine(::makeCursorLine(_tracks.size()));
}

void CompositionScene::insertFragment(const void* voiceId, const void* trackId, size_t offset, const std::shared_ptr<aulos::SequenceData>& sequence)
{
	const auto track = std::find_if(_tracks.begin(), _tracks.end(), [trackId](const auto& trackPtr) { return trackPtr->_background->trackId() == trackId; });
	assert(track != _tracks.end());
	const auto newItem = addFragmentItem(voiceId, **track, offset, sequence);
	const auto minCompositionLength = offset + newItem->fragmentLength() + kExtraLength;
	if (minCompositionLength > _timelineItem->compositionLength())
		setCompositionLength(minCompositionLength);
}

void CompositionScene::removeFragment(const void* trackId, size_t offset)
{
	const auto track = std::find_if(_tracks.begin(), _tracks.end(), [trackId](const auto& trackPtr) { return trackPtr->_background->trackId() == trackId; });
	assert(track != _tracks.end());
	const auto fragment = (*track)->_fragments.find(offset);
	assert(fragment != (*track)->_fragments.end());
	removeItem(fragment->second);
	fragment->second->deleteLater();
	(*track)->_fragments.erase(fragment);
}

void CompositionScene::removeTrack(const void* voiceId, const void* trackId)
{
	const auto [voice, voiceOffset] = ::findVoice(_voices, voiceId);
	assert(voice != _voices.end());
	const auto track = std::find_if(_tracks.begin(), _tracks.end(), [trackId](const auto& trackPtr) { return trackPtr->_background->trackId() == trackId; });
	assert(track != _tracks.end());
	assert((*track)->_background->parentItem() == voice->get());
	removeItem((*track)->_background);
	(*voice)->setTrackCount((*voice)->trackCount() - 1);
	std::for_each(std::next(voice), _voices.cend(), [index = voiceOffset + (*voice)->trackCount()](const auto& voiceItem) mutable {
		voiceItem->setPos(0, index * kTrackHeight);
		index += voiceItem->trackCount();
	});
	if (const auto nextTrack = std::next(track); nextTrack != _tracks.end())
	{
		const auto index = (*track)->_background->trackIndex() - voiceOffset;
		std::for_each(nextTrack, std::find_if(nextTrack, _tracks.end(), [](const auto& trackPtr) { return trackPtr->_background->isFirstTrack(); }), [index = index](const auto& trackItem) mutable {
			trackItem->_background->setPos(0, index * kTrackHeight);
			++index;
		});
		if (!index)
			(*nextTrack)->_background->setFirstTrack(true);
		std::for_each(nextTrack, _tracks.end(), [](const auto& trackItem) {
			trackItem->_background->setTrackIndex(trackItem->_background->trackIndex() - 1);
			for (const auto& fragment : trackItem->_fragments)
				fragment.second->update();
		});
	}
	_tracks.erase(track);
	_addVoiceItem->setPos(0, _tracks.size() * kTrackHeight);
	_cursorItem->setLine(::makeCursorLine(_tracks.size()));
	updateSceneRect(_timelineItem->compositionLength());
}

void CompositionScene::removeVoice(const void* voiceId)
{
	const auto [voice, voiceOffset] = ::findVoice(_voices, voiceId);
	assert(voice != _voices.end());
	removeItem(voice->get());
	std::for_each(std::next(voice), _voices.cend(), [index = voiceOffset](const auto& voiceItem) mutable {
		voiceItem->setPos(0, index * kTrackHeight);
		voiceItem->setVoiceIndex(voiceItem->voiceIndex() - 1);
		index += voiceItem->trackCount();
	});
	const auto tracksBegin = _tracks.begin() + voiceOffset;
	const auto tracksEnd = tracksBegin + (*voice)->trackCount();
	std::for_each(tracksEnd, _tracks.end(), [index = voiceOffset](const auto& trackItem) mutable {
		trackItem->_background->setTrackIndex(index);
		++index;
	});
	_tracks.erase(tracksBegin, tracksEnd);
	_voices.erase(voice);
	_addVoiceItem->setIndex(_voices.size());
	_addVoiceItem->setPos(0, _tracks.size() * kTrackHeight);
	_cursorItem->setLine(::makeCursorLine(_tracks.size()));
	updateSceneRect(_timelineItem->compositionLength());
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

		const auto voiceItem = addVoiceItem(partData->_voice.get(), QString::fromStdString(partData->_voice->_name), partData->_tracks.size());

		const auto trackIndexBase = _tracks.size();
		for (auto i = partData->_tracks.cbegin(); i != partData->_tracks.cend(); ++i)
		{
			const auto trackOffset = i - partData->_tracks.cbegin();
			const auto& track = _tracks.emplace_back(std::make_unique<Track>());
			track->_background = addTrackItem(voiceItem, i->get());
			track->_background->setFirstTrack(trackOffset == 0);
			track->_background->setPos(0, trackOffset * kTrackHeight);
			track->_background->setTrackIndex(trackIndexBase + trackOffset);
			if (trackOffset > 0)
				track->_background->stackBefore(_tracks[_tracks.size() - 2]->_background);
			for (const auto& fragment : (*i)->_fragments)
			{
				const auto fragmentItem = addFragmentItem(voiceItem->voiceId(), *track, fragment.first, fragment.second);
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

void CompositionScene::updateVoice(const void* id, const std::string& name)
{
	const auto i = std::find_if(_voices.cbegin(), _voices.cend(), [id](const auto& voiceItem) { return voiceItem->voiceId() == id; });
	assert(i != _voices.cend());
	(*i)->setVoiceName(QString::fromStdString(name));
	if (const auto width = requiredVoiceColumnWidth(); width != _voiceColumnWidth)
	{
		_voiceColumnWidth = width;
		updateSceneRect(_timelineItem->compositionLength());
		setVoiceColumnWidth(width);
	}
}

void CompositionScene::setCompositionLength(size_t length)
{
	updateSceneRect(length);
	_timelineItem->setCompositionLength(length);
	for (const auto& track : _tracks)
		track->_background->setTrackLength(length);
}

FragmentItem* CompositionScene::addFragmentItem(const void* voiceId, Track& track, size_t offset, const std::shared_ptr<aulos::SequenceData>& sequence)
{
	const auto item = new FragmentItem{ track._background, offset, sequence };
	item->setPos(offset * kStepWidth, 0);
	connect(item, &FragmentItem::fragmentActionRequested, [this, voiceId, trackId = track._background->trackId()](size_t offset) {
		emit fragmentActionRequested(voiceId, trackId, offset);
	});
	connect(item, &FragmentItem::fragmentMenuRequested, [this, voiceId, trackId = track._background->trackId()](size_t offset, const QPoint& pos) {
		emit fragmentMenuRequested(voiceId, trackId, offset, pos);
	});
	const auto i = track._fragments.emplace(offset, item).first;
	if (i != track._fragments.begin())
		std::prev(i)->second->stackBefore(i->second);
	if (const auto next = std::next(i); next != track._fragments.end())
		i->second->stackBefore(next->second);
	return item;
}

TrackItem* CompositionScene::addTrackItem(VoiceItem* voiceItem, const void* trackId)
{
	const auto trackItem = new TrackItem{ trackId, voiceItem };
	connect(trackItem, &TrackItem::trackActionRequested, [this, voiceId = voiceItem->voiceId()](const void* trackId) {
		emit trackActionRequested(voiceId, trackId);
	});
	connect(trackItem, &TrackItem::trackMenuRequested, [this, voiceId = voiceItem->voiceId()](const void* trackId, size_t offset, const QPoint& pos) {
		emit trackMenuRequested(voiceId, trackId, offset, pos);
	});
	return trackItem;
}

VoiceItem* CompositionScene::addVoiceItem(const void* id, const QString& name, size_t trackCount)
{
	const auto voiceIndex = _voices.size();
	const auto voiceItem = _voices.emplace_back(std::make_unique<VoiceItem>(id)).get();
	voiceItem->setPos(0, _tracks.size() * kTrackHeight);
	voiceItem->setTrackCount(trackCount);
	voiceItem->setVoiceIndex(voiceIndex);
	voiceItem->setVoiceName(name);
	connect(voiceItem, &VoiceItem::voiceActionRequested, this, &CompositionScene::voiceActionRequested);
	connect(voiceItem, &VoiceItem::voiceMenuRequested, this, &CompositionScene::voiceMenuRequested);
	return voiceItem;
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
