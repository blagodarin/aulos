// This file is part of the Aulos toolkit.
// Copyright (C) Sergei Blagodarin.
// SPDX-License-Identifier: Apache-2.0

#include "sequence_scene.hpp"

#include "../elusive_item.hpp"
#include "../theme.hpp"
#include "key_item.hpp"
#include "pianoroll_item.hpp"
#include "sound_item.hpp"

#include <algorithm>
#include <cassert>
#include <cmath>

SequenceScene::SequenceScene(QObject* parent)
	: QGraphicsScene{ parent }
{
	setBackgroundBrush(kBackgroundColor);
	for (size_t i = 0; i < seir::synth::kNoteCount; ++i)
	{
		const auto note = static_cast<seir::synth::Note>(i);
		const auto keyItem = new KeyItem{ note };
		addItem(keyItem);
		connect(keyItem, &KeyItem::activated, [this, note] { emit noteActivated(note); });
	}
	_pianorollItem = std::make_unique<PianorollItem>();
	_pianorollItem->setPos(kWhiteKeyWidth, 0);
	addItem(_pianorollItem.get());
	connect(_pianorollItem.get(), &PianorollItem::newSoundRequested, this, &SequenceScene::insertingSound);
	_rightBoundItem = new ElusiveItem{ _pianorollItem.get() };
	_rightBoundItem->setHeight(_pianorollItem->boundingRect().height());
	connect(_rightBoundItem, &ElusiveItem::elude, [this] { setPianorollLength(_pianorollItem->stepCount() + kPianorollStride); });
}

SequenceScene::~SequenceScene()
{
	removeSoundItems();
	removeItem(_pianorollItem.get());
}

void SequenceScene::insertSound(size_t offset, seir::synth::Note note)
{
	insertNewSound(offset, note);
	emit noteActivated(note);
}

void SequenceScene::removeSound(size_t offset, seir::synth::Note note)
{
	const auto range = _soundItems.equal_range(offset);
	assert(range.first != range.second);
	const auto item = std::find_if(range.first, range.second, [note](const auto& sound) { return sound.second->note() == note; });
	assert(item != range.second);
	removeItem(item->second.get());
	item->second.release()->deleteLater();
	_soundItems.erase(item);
}

qreal SequenceScene::setSequence(const seir::synth::SequenceData& sequence, const QSize& viewSize)
{
	removeSoundItems();
	size_t offset = 0;
	for (const auto& sound : sequence._sounds)
	{
		offset += sound._delay;
		insertNewSound(offset, sound._note);
	}
	setPianorollLength(std::max((offset + kPianorollStride) / kPianorollStride * kPianorollStride, viewSize.width() / static_cast<size_t>(kNoteWidth) + 1));
	const auto heightDifference = std::lround(sceneRect().height() - viewSize.height());
	if (heightDifference <= 0 || _soundItems.empty())
		return 0.5;
	auto rect = _soundItems.cbegin()->second->sceneBoundingRect();
	std::for_each(std::next(_soundItems.cbegin()), _soundItems.cend(), [&rect](const auto& soundItem) { rect |= soundItem.second->sceneBoundingRect(); });
	return (rect.center().y() - viewSize.height() / 2) / heightDifference;
}

void SequenceScene::insertNewSound(size_t offset, seir::synth::Note note)
{
	const auto soundItem = _soundItems.emplace(offset, std::make_unique<SoundItem>(offset, note, _pianorollItem.get()))->second.get();
	connect(soundItem, &SoundItem::playRequested, [this, soundItem] { emit noteActivated(soundItem->note()); });
	connect(soundItem, &SoundItem::removeRequested, [this, offset, note] { emit removingSound(offset, note); });
}

void SequenceScene::removeSoundItems()
{
	// Qt documentation says it's more efficient to remove items from the scene before destroying them.
	for (const auto& sound : _soundItems)
		removeItem(sound.second.get());
	_soundItems.clear();
}

void SequenceScene::setPianorollLength(size_t steps)
{
	setSceneRect({ 0, 0, kWhiteKeyWidth + steps * kNoteWidth, _pianorollItem->boundingRect().height() });
	_pianorollItem->setStepCount(steps);
	_rightBoundItem->setPos(_pianorollItem->boundingRect().topRight());
}
