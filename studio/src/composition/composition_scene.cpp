// This file is part of the Aulos toolkit.
// Copyright (C) Sergei Blagodarin.
// SPDX-License-Identifier: Apache-2.0

#include "composition_scene.hpp"

#include "../elusive_item.hpp"
#include "../theme.hpp"
#include "add_voice_item.hpp"
#include "cursor_item.hpp"
#include "fragment_item.hpp"
#include "loop_item.hpp"
#include "timeline_item.hpp"
#include "track_item.hpp"
#include "voice_item.hpp"

#include <aulos/data.hpp>

#include <cassert>
#include <numeric>
#include <vector>

#include <QGraphicsRectItem>

namespace
{
	constexpr qreal kDefaultZValue = 0;
	constexpr qreal kHighlightZValue = 1;
	constexpr qreal kCursorZValue = 2;

	const std::array<QString, 7> kNoteNameTemplates{
		QStringLiteral("C<%1>%2</%1>"),
		QStringLiteral("D<%1>%2</%1>"),
		QStringLiteral("E<%1>%2</%1>"),
		QStringLiteral("F<%1>%2</%1>"),
		QStringLiteral("G<%1>%2</%1>"),
		QStringLiteral("A<%1>%2</%1>"),
		QStringLiteral("B<%1>%2</%1>"),
	};

	struct NoteInfo
	{
		unsigned _base = 0;
		bool _extra = false;
	};

	const std::array<NoteInfo, 12> kNoteInfo{
		NoteInfo{ 0, false }, // C
		NoteInfo{ 0, true },  // C#
		NoteInfo{ 1, false }, // D
		NoteInfo{ 1, true },  // D#
		NoteInfo{ 2, false }, // E
		NoteInfo{ 3, false }, // F
		NoteInfo{ 3, true },  // F#
		NoteInfo{ 4, false }, // G
		NoteInfo{ 4, true },  // G#
		NoteInfo{ 5, false }, // A
		NoteInfo{ 5, true },  // A#
		NoteInfo{ 6, false }, // B
	};

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
}

class CompositionItem final : public QGraphicsItem
{
private:
	QRectF boundingRect() const override { return {}; }
	void paint(QPainter*, const QStyleOptionGraphicsItem*, QWidget*) override {}
};

struct CompositionScene::Track
{
	QGraphicsScene& _scene;
	TrackItem* _background = nullptr;
	std::map<size_t, FragmentItem*> _fragments;

	explicit Track(QGraphicsScene& scene)
		: _scene{ scene } {}

	~Track()
	{
		_background->deleteLater();
		_scene.removeItem(_background);
		for (auto& fragment : _fragments)
		{
			fragment.second->deleteLater();
			_scene.removeItem(fragment.second);
		}
	}

	void setIndex(size_t index)
	{
		const auto y = index * kTrackHeight;
		_background->setPos(0, y);
		_background->setTrackIndex(index);
		for (const auto& fragment : _fragments)
		{
			fragment.second->setPos(fragment.second->fragmentOffset() * kStepWidth, y);
			fragment.second->setTrackIndex(index);
		}
	}
};

CompositionScene::CompositionScene(QObject* parent)
	: QGraphicsScene{ parent }
	, _addVoiceItem{ std::make_unique<AddVoiceItem>() }
	, _compositionItem{ std::make_unique<CompositionItem>() }
	, _timelineItem{ new TimelineItem{ _compositionItem.get() } }
	, _rightBoundItem{ new ElusiveItem{ _compositionItem.get() } }
	, _cursorItem{ new CursorItem{ _compositionItem.get() } }
	, _loopItem{ new LoopItem{ _compositionItem.get() } }
	, _voiceColumnWidth{ kMinVoiceItemWidth }
{
	setBackgroundBrush(kBackgroundColor);
	_addVoiceItem->setWidth(_voiceColumnWidth);
	connect(_addVoiceItem.get(), &ButtonItem::activated, this, &CompositionScene::newVoiceRequested);
	_compositionItem->setPos(_voiceColumnWidth, kCompositionHeaderHeight);
	_timelineItem->setPos(0, -kCompositionHeaderHeight);
	connect(_timelineItem, &TimelineItem::menuRequested, this, &CompositionScene::timelineMenuRequested);
	_rightBoundItem->setPos(_timelineItem->pos() + _timelineItem->boundingRect().topRight());
	connect(_rightBoundItem, &ElusiveItem::elude, [this] {
		const auto length = _timelineItem->compositionLength();
		const auto speed = _timelineItem->compositionSpeed();
		setCompositionLength(length + (speed - length % speed));
	});
	_cursorItem->setVisible(false);
	_cursorItem->setZValue(kCursorZValue);
	_loopItem->setVisible(false);
	connect(_loopItem, &LoopItem::menuRequested, this, &CompositionScene::loopMenuRequested);

	QTextOption textOption;
	textOption.setWrapMode(QTextOption::NoWrap);
	for (size_t octave = 0; octave <= 9; ++octave)
	{
		for (size_t note = 0; note < 7; ++note)
		{
			auto& text = _baseNoteNames[7 * octave + note];
			text = std::make_shared<QStaticText>(kNoteNameTemplates[note].arg("sub").arg(octave));
			text->setTextFormat(Qt::RichText);
			text->setTextOption(textOption);
			text->setTextWidth(kStepWidth);
		}
	}
	for (size_t note = 0; note < 7; ++note)
	{
		auto& text = _extraNoteNames[note];
		text = std::make_shared<QStaticText>(kNoteNameTemplates[note].arg("sup").arg('#'));
		text->setTextFormat(Qt::RichText);
		text->setTextOption(textOption);
		text->setTextWidth(kStepWidth);
	}
}

CompositionScene::~CompositionScene() = default;

void CompositionScene::addTrack(const void* voiceId, const void* trackId)
{
	const auto [voiceIt, voiceOffset] = ::findVoice(_voices, voiceId);
	assert(voiceIt != _voices.end());
	const auto trackOffset = (*voiceIt)->trackCount();
	const auto trackIndex = voiceOffset + trackOffset;
	const auto trackIt = addTrackItem(voiceId, trackId, trackIndex, trackOffset == 0);
	(*trackIt)->_background->setTrackLength(_timelineItem->compositionLength());
	(*voiceIt)->setTrackCount(trackOffset + 1);
	std::for_each(std::next(voiceIt), _voices.cend(), [index = trackIndex + 1](const auto& voiceItem) mutable {
		voiceItem->setPos(0, kCompositionHeaderHeight + index * kTrackHeight);
		index += voiceItem->trackCount();
	});
	std::for_each(std::next(trackIt), _tracks.end(), [index = trackIndex + 1](const auto& trackPtr) mutable { trackPtr->setIndex(index++); });
	_addVoiceItem->setPos(0, kCompositionHeaderHeight + _tracks.size() * kTrackHeight);
	_cursorItem->setTrackCount(_tracks.size());
	_loopItem->setPos(_composition->_loopOffset * kStepWidth, _tracks.size() * kTrackHeight + kLoopItemOffset);
	updateSceneRect(_timelineItem->compositionLength());
}

void CompositionScene::appendPart(const std::shared_ptr<aulos::PartData>& partData)
{
	assert(partData->_tracks.size() == 1);
	assert(partData->_tracks.front()->_fragments.empty());

	const auto voiceItem = addVoiceItem(partData->_voice.get(), QString::fromStdString(partData->_voiceName), partData->_tracks.size());

	const auto trackIt = addTrackItem(partData->_voice.get(), partData->_tracks.front().get(), _tracks.size(), true);
	(*trackIt)->_background->setTrackLength(_timelineItem->compositionLength());

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
	voiceItem->stackBefore(_addVoiceItem.get());

	_addVoiceItem->setIndex(_voices.size());
	_addVoiceItem->setPos(0, kCompositionHeaderHeight + _tracks.size() * kTrackHeight);
	_cursorItem->setTrackCount(_tracks.size());
	_loopItem->setPos(_composition->_loopOffset * kStepWidth, _tracks.size() * kTrackHeight + kLoopItemOffset);
}

void CompositionScene::insertFragment(const void* voiceId, const void* trackId, size_t offset, const std::shared_ptr<aulos::SequenceData>& sequence)
{
	const auto trackIt = std::find_if(_tracks.begin(), _tracks.end(), [trackId](const auto& trackPtr) { return trackPtr->_background->trackId() == trackId; });
	assert(trackIt != _tracks.end());
	addFragmentItem(voiceId, trackIt, offset, sequence);
}

void CompositionScene::removeFragment(const void* trackId, size_t offset)
{
	const auto trackIt = std::find_if(_tracks.begin(), _tracks.end(), [trackId](const auto& trackPtr) { return trackPtr->_background->trackId() == trackId; });
	assert(trackIt != _tracks.end());
	const auto fragmentIt = (*trackIt)->_fragments.find(offset);
	assert(fragmentIt != (*trackIt)->_fragments.end());
	fragmentIt->second->deleteLater();
	removeItem(fragmentIt->second);
	(*trackIt)->_fragments.erase(fragmentIt);
}

void CompositionScene::removeTrack(const void* voiceId, const void* trackId)
{
	const auto [voiceIt, voiceOffset] = ::findVoice(_voices, voiceId);
	assert(voiceIt != _voices.end());
	const auto trackIt = std::find_if(_tracks.begin(), _tracks.end(), [trackId](const auto& trackPtr) { return trackPtr->_background->trackId() == trackId; });
	assert(trackIt != _tracks.end());
	(*voiceIt)->setTrackCount((*voiceIt)->trackCount() - 1);
	std::for_each(std::next(voiceIt), _voices.cend(), [index = voiceOffset + (*voiceIt)->trackCount()](const auto& voiceItem) mutable {
		voiceItem->setPos(0, kCompositionHeaderHeight + index * kTrackHeight);
		index += voiceItem->trackCount();
	});
	if (const auto nextTrackIt = std::next(trackIt); nextTrackIt != _tracks.end())
	{
		const auto trackIndex = (*trackIt)->_background->trackIndex();
		if (trackIndex == voiceOffset)
			(*nextTrackIt)->_background->setFirstTrack(true);
		std::for_each(nextTrackIt, _tracks.end(), [index = trackIndex](const auto& trackPtr) mutable { trackPtr->setIndex(index++); });
	}
	_tracks.erase(trackIt);
	_addVoiceItem->setPos(0, kCompositionHeaderHeight + _tracks.size() * kTrackHeight);
	_cursorItem->setTrackCount(_tracks.size());
	_loopItem->setPos(_composition->_loopOffset * kStepWidth, _tracks.size() * kTrackHeight + kLoopItemOffset);
	updateSceneRect(_timelineItem->compositionLength());
	if (trackId == _selectedTrackId)
	{
		_selectedTrackId = nullptr;
		_selectedSequenceId = nullptr;
		emit sequenceSelected(_selectedVoiceId, nullptr, nullptr);
	}
}

void CompositionScene::removeVoice(const void* voiceId)
{
	const auto [voiceIt, voiceOffset] = ::findVoice(_voices, voiceId);
	assert(voiceIt != _voices.end());
	removeItem(voiceIt->get());
	std::for_each(std::next(voiceIt), _voices.cend(), [index = voiceOffset](const auto& voiceItem) mutable {
		voiceItem->setPos(0, kCompositionHeaderHeight + index * kTrackHeight);
		voiceItem->setVoiceIndex(voiceItem->voiceIndex() - 1);
		index += voiceItem->trackCount();
	});
	const auto tracksBegin = _tracks.begin() + voiceOffset;
	const auto tracksEnd = tracksBegin + (*voiceIt)->trackCount();
	std::for_each(tracksEnd, _tracks.end(), [index = voiceOffset](const auto& trackPtr) mutable { trackPtr->setIndex(index++); });
	_tracks.erase(tracksBegin, tracksEnd);
	_voices.erase(voiceIt);
	_addVoiceItem->setIndex(_voices.size());
	_addVoiceItem->setPos(0, kCompositionHeaderHeight + _tracks.size() * kTrackHeight);
	_cursorItem->setTrackCount(_tracks.size());
	_loopItem->setPos(_composition->_loopOffset * kStepWidth, _tracks.size() * kTrackHeight + kLoopItemOffset);
	updateSceneRect(_timelineItem->compositionLength());
	if (voiceId == _selectedVoiceId)
	{
		_selectedVoiceId = nullptr;
		_selectedTrackId = nullptr;
		_selectedSequenceId = nullptr;
		emit sequenceSelected(nullptr, nullptr, nullptr);
	}
}

void CompositionScene::reset(const std::shared_ptr<aulos::CompositionData>& composition, size_t viewWidth)
{
	if (_composition)
	{
		_tracks.clear();
		removeItem(_compositionItem.get());
		removeItem(_addVoiceItem.get());
		_voices.clear();
		clear();
	}
	_composition = composition;
	if (_composition)
	{
		auto compositionLength = viewWidth / static_cast<size_t>(kStepWidth) + 1;
		_voices.reserve(_composition->_parts.size());
		for (const auto& partData : _composition->_parts)
		{
			assert(!partData->_tracks.empty());
			const auto voiceItem = addVoiceItem(partData->_voice.get(), QString::fromStdString(partData->_voiceName), partData->_tracks.size());
			for (const auto& trackData : partData->_tracks)
			{
				const auto trackIt = addTrackItem(partData->_voice.get(), trackData.get(), _tracks.size(), trackData == partData->_tracks.front());
				for (const auto& fragment : trackData->_fragments)
				{
					const auto fragmentItem = addFragmentItem(voiceItem->voiceId(), trackIt, fragment.first, fragment.second);
					compositionLength = std::max(compositionLength, fragmentItem->fragmentOffset() + fragmentItem->fragmentLength());
				}
			}
		}

		_timelineItem->setCompositionSpeed(_composition->_speed);
		_timelineItem->setCompositionLength(compositionLength);
		_timelineItem->setCompositionOffset(0);
		for (const auto& track : _tracks)
			track->_background->setTrackLength(compositionLength);
		_rightBoundItem->setPos(_timelineItem->pos() + _timelineItem->boundingRect().topRight());
		_addVoiceItem->setIndex(_voices.size());
		_addVoiceItem->setPos(0, kCompositionHeaderHeight + _tracks.size() * kTrackHeight);
		_cursorItem->setTrackCount(_tracks.size());
		_cursorItem->setVisible(false);

		setVoiceColumnWidth(requiredVoiceColumnWidth());
		updateLoop();
		updateSceneRect(compositionLength);
		for (auto i = _voices.crbegin(); i != _voices.crend(); ++i)
			addItem(i->get());
		addItem(_addVoiceItem.get());
		addItem(_compositionItem.get());
	}
	if (_selectedVoiceId || _selectedTrackId || _selectedSequenceId)
	{
		_selectedVoiceId = nullptr;
		_selectedTrackId = nullptr;
		_selectedSequenceId = nullptr;
		emit sequenceSelected(nullptr, nullptr, nullptr);
	}
}

void CompositionScene::selectSequence(const void* voiceId, const void* trackId, const void* sequenceId)
{
	if (_selectedVoiceId != voiceId)
	{
		if (_selectedVoiceId)
			highlightVoice(_selectedVoiceId, false);
		if (voiceId)
			highlightVoice(voiceId, true);
		_selectedVoiceId = voiceId;
	}
	if (_selectedTrackId && _selectedTrackId != trackId)
		highlightSequence(_selectedTrackId, nullptr);
	_selectedTrackId = trackId;
	_selectedSequenceId = sequenceId;
	if (trackId)
		highlightSequence(trackId, sequenceId);
	emit sequenceSelected(voiceId, trackId, sequenceId);
}

float CompositionScene::selectedTrackWeight() const
{
	if (!_selectedTrackId)
		return 1.f;
	assert(_selectedVoiceId);
	const auto partIt = std::find_if(_composition->_parts.cbegin(), _composition->_parts.cend(), [this](const auto& partData) { return partData->_voice.get() == _selectedVoiceId; });
	assert(partIt != _composition->_parts.cend());
	const auto trackIt = std::find_if((*partIt)->_tracks.cbegin(), (*partIt)->_tracks.cend(), [this](const auto& trackData) { return trackData.get() == _selectedTrackId; });
	assert(trackIt != (*partIt)->_tracks.cend());
	unsigned totalWeight = 0;
	for (const auto& part : _composition->_parts)
		totalWeight = std::accumulate(part->_tracks.begin(), part->_tracks.end(), totalWeight, [](unsigned weight, const auto& trackData) { return weight + trackData->_weight; });
	return static_cast<float>((*trackIt)->_weight) / totalWeight;
}

QRectF CompositionScene::setCurrentStep(double step)
{
	// Moving cursor leaves artifacts if the view is being scrolled.
	// The moved-from area should be updated to clean them up.
	const auto updateRect = _cursorItem->mapRectToScene(_cursorItem->boundingRect());
	_cursorItem->setPos(step * kStepWidth, -kTimelineHeight);
	update(updateRect);
	return _cursorItem->sceneBoundingRect();
}

void CompositionScene::setSpeed(unsigned speed)
{
	_timelineItem->setCompositionSpeed(speed);
}

void CompositionScene::showCursor(bool visible)
{
	_cursorItem->setVisible(visible);
}

size_t CompositionScene::startOffset() const
{
	return _timelineItem->compositionOffset();
}

void CompositionScene::updateLoop()
{
	_loopItem->setLoopLength(_composition->_loopLength);
	_loopItem->setPos(_composition->_loopOffset * kStepWidth, _tracks.size() * kTrackHeight + kLoopItemOffset);
	_loopItem->setVisible(_composition->_loopLength > 0);
}

void CompositionScene::updateSelectedSequence(const std::shared_ptr<aulos::SequenceData>& sequence)
{
	assert(_selectedTrackId);
	const auto trackIt = std::find_if(_tracks.begin(), _tracks.end(), [this](const auto& trackPtr) { return trackPtr->_background->trackId() == _selectedTrackId; });
	assert(trackIt != _tracks.end());
	const auto texts = makeSequenceTexts(*sequence);
	for (const auto& fragment : (*trackIt)->_fragments)
		if (fragment.second->sequenceId() == sequence.get())
			fragment.second->setSequence(texts);
}

void CompositionScene::updateVoice(const void* id, const std::string& name)
{
	const auto voiceIt = std::find_if(_voices.cbegin(), _voices.cend(), [id](const auto& voiceItem) { return voiceItem->voiceId() == id; });
	assert(voiceIt != _voices.cend());
	(*voiceIt)->setVoiceName(QString::fromStdString(name));
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
	_rightBoundItem->setPos(_timelineItem->pos() + _timelineItem->boundingRect().topRight());
}

FragmentItem* CompositionScene::addFragmentItem(const void* voiceId, TrackIterator trackIt, size_t offset, const std::shared_ptr<aulos::SequenceData>& sequence)
{
	const auto trackIndex = (*trackIt)->_background->trackIndex();
	const auto item = new FragmentItem{ trackIndex, offset, sequence.get(), _compositionItem.get() };
	item->setHighlighted(sequence.get() == _selectedSequenceId);
	item->setPos(offset * kStepWidth, trackIndex * kTrackHeight);
	item->setSequence(makeSequenceTexts(*sequence));
	connect(item, &FragmentItem::fragmentMenuRequested, [this, voiceId, trackId = (*trackIt)->_background->trackId()](size_t offset, const QPoint& pos) {
		emit fragmentMenuRequested(voiceId, trackId, offset, pos);
	});
	connect(item, &FragmentItem::sequenceSelected, [this, voiceId, trackId = (*trackIt)->_background->trackId()](const void* sequenceId) { selectSequence(voiceId, trackId, sequenceId); });
	const auto fragmentIt = (*trackIt)->_fragments.emplace(offset, item).first;
	if (const auto nextFragmentIt = std::next(fragmentIt); nextFragmentIt != (*trackIt)->_fragments.end())
		fragmentIt->second->stackBefore(nextFragmentIt->second);
	else if (const auto nextFragmentTrackIt = std::find_if(std::next(trackIt), _tracks.end(), [](const auto& trackPtr) { return !trackPtr->_fragments.empty(); }); nextFragmentTrackIt != _tracks.end())
		fragmentIt->second->stackBefore((*nextFragmentTrackIt)->_fragments.cbegin()->second);
	else
		fragmentIt->second->stackBefore(_rightBoundItem);
	return item;
}

CompositionScene::TrackIterator CompositionScene::addTrackItem(const void* voiceId, const void* trackId, size_t trackIndex, bool isFirstTrack)
{
	assert(trackIndex <= _tracks.size());
	const auto trackIt = _tracks.emplace(_tracks.begin() + trackIndex, std::make_unique<Track>(*this));
	(*trackIt)->_background = new TrackItem{ trackId, _compositionItem.get() };
	(*trackIt)->_background->setFirstTrack(isFirstTrack);
	(*trackIt)->_background->setPos(0, trackIndex * kTrackHeight);
	(*trackIt)->_background->setTrackIndex(trackIndex);
	if (const auto nextTrackIt = std::next(trackIt); nextTrackIt != _tracks.end())
		(*trackIt)->_background->stackBefore((*nextTrackIt)->_background);
	else if (const auto firstFragmentTrackIt = std::find_if(_tracks.begin(), trackIt, [](const auto& trackPtr) { return !trackPtr->_fragments.empty(); }); firstFragmentTrackIt != trackIt)
		(*trackIt)->_background->stackBefore((*firstFragmentTrackIt)->_fragments.cbegin()->second);
	else
		(*trackIt)->_background->stackBefore(_rightBoundItem);
	connect((*trackIt)->_background, &TrackItem::trackActionRequested, [this, voiceId](const void* trackId) {
		emit trackActionRequested(voiceId, trackId);
	});
	connect((*trackIt)->_background, &TrackItem::trackMenuRequested, [this, voiceId](const void* trackId, size_t offset, const QPoint& pos) {
		emit trackMenuRequested(voiceId, trackId, offset, pos);
	});
	return trackIt;
}

VoiceItem* CompositionScene::addVoiceItem(const void* id, const QString& name, size_t trackCount)
{
	const auto voiceIndex = _voices.size();
	const auto voiceItem = _voices.emplace_back(std::make_unique<VoiceItem>(id)).get();
	voiceItem->setPos(0, kCompositionHeaderHeight + _tracks.size() * kTrackHeight);
	voiceItem->setTrackCount(trackCount);
	voiceItem->setVoiceIndex(voiceIndex);
	voiceItem->setVoiceName(name);
	connect(voiceItem, &VoiceItem::voiceActionRequested, this, &CompositionScene::voiceActionRequested);
	connect(voiceItem, &VoiceItem::voiceMenuRequested, this, &CompositionScene::voiceMenuRequested);
	connect(voiceItem, &VoiceItem::voiceSelected, [this](const void* voiceId) { selectSequence(voiceId, nullptr, nullptr); });
	return voiceItem;
}

void CompositionScene::highlightSequence(const void* trackId, const void* sequenceId)
{
	const auto trackIt = std::find_if(_tracks.begin(), _tracks.end(), [trackId](const auto& trackPtr) { return trackPtr->_background->trackId() == trackId; });
	assert(trackIt != _tracks.end());
	for (const auto& fragment : (*trackIt)->_fragments)
	{
		const auto shouldBeHighlighted = fragment.second->sequenceId() == sequenceId;
		if (shouldBeHighlighted == fragment.second->isHighlighted())
			continue;
		fragment.second->setHighlighted(shouldBeHighlighted);
		fragment.second->setZValue(shouldBeHighlighted ? kHighlightZValue : kDefaultZValue);
	}
}

void CompositionScene::highlightVoice(const void* id, bool highlight)
{
	const auto voiceIt = std::find_if(_voices.begin(), _voices.end(), [id](const auto& voicePtr) { return voicePtr->voiceId() == id; });
	assert(voiceIt != _voices.end());
	(*voiceIt)->setHighlighted(highlight);
	(*voiceIt)->setZValue(highlight ? kHighlightZValue : kDefaultZValue);
}

std::vector<FragmentSound> CompositionScene::makeSequenceTexts(const aulos::SequenceData& sequence) const
{
	std::vector<FragmentSound> result;
	result.reserve(sequence._sounds.size()); // A reasonable guess.
	for (const auto& sound : sequence._sounds)
	{
		if (!sound._delay && !result.empty())
			continue; // Show only the first (the highest) note at one position.
		const auto& info = kNoteInfo[static_cast<size_t>(sound._note) % 12];
		const auto octave = static_cast<size_t>(sound._note) / 12;
		result.emplace_back(sound._delay, _baseNoteNames[7 * octave + info._base]);
		if (info._extra)
			result.emplace_back(0, _extraNoteNames[info._base]);
	}
	return result;
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
	_compositionItem->setPos(width, kCompositionHeaderHeight);
}

void CompositionScene::updateSceneRect(size_t compositionLength)
{
	setSceneRect(0, 0, _voiceColumnWidth + compositionLength * kStepWidth, kCompositionHeaderHeight + _tracks.size() * kTrackHeight + std::max(kAddVoiceItemHeight, kCompositionFooterHeight));
}
