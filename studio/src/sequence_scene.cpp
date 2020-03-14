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

#include "sequence_scene.hpp"

#include "colors.hpp"
#include "elusive_item.hpp"
#include "key_item.hpp"
#include "pianoroll_item.hpp"
#include "sound_item.hpp"
#include "utils.hpp"

#include <cassert>

SequenceScene::SequenceScene(QObject* parent)
	: QGraphicsScene{ parent }
{
	setBackgroundBrush(kBackgroundColor);
	for (int i = 0; i < 120; ++i)
	{
		const auto note = static_cast<aulos::Note>(i);
		const auto keyItem = new KeyItem{ note };
		addItem(keyItem);
		connect(keyItem, &KeyItem::activated, [this, note] { emit noteActivated(note); });
	}
	_pianorollItem = std::make_unique<PianorollItem>();
	_pianorollItem->setPos(kWhiteKeyWidth, 0);
	addItem(_pianorollItem.get());
	connect(_pianorollItem.get(), &PianorollItem::newSoundRequested, [this](size_t offset, aulos::Note note) {
		if (_editable)
			insertSound(offset, note);
		else
			emit insertingSound(offset, note);
	});
	_rightBoundItem = new ElusiveItem{ _pianorollItem.get() };
	_rightBoundItem->setHeight(_pianorollItem->boundingRect().height());
	connect(_rightBoundItem, &ElusiveItem::elude, [this] { setPianorollLength(_pianorollItem->stepCount() + kPianorollStride); });
}

SequenceScene::~SequenceScene()
{
	removeSoundItems();
	removeItem(_pianorollItem.get());
}

void SequenceScene::insertSound(size_t offset, aulos::Note note)
{
	if (const auto i = _soundItems.find(offset); i != _soundItems.end())
		i->second->setNote(note);
	else
		insertNewSound(offset, note);
	emit noteActivated(note);
}

void SequenceScene::removeSound(size_t offset)
{
	const auto i = _soundItems.find(offset);
	assert(i != _soundItems.end());
	removeItem(i->second.get());
	i->second.release()->deleteLater();
	_soundItems.erase(i);
}

aulos::SequenceData SequenceScene::sequence() const
{
	aulos::SequenceData result;
	size_t lastOffset = 0;
	for (const auto& soundItem : _soundItems)
	{
		result._sounds.emplace_back(soundItem.first - lastOffset, soundItem.second->note());
		lastOffset = soundItem.first;
	}
	return result;
}

qreal SequenceScene::setSequence(const aulos::SequenceData& sequence, const QSize& viewSize)
{
	removeSoundItems();
	size_t offset = 0;
	for (const auto& sound : sequence._sounds)
	{
		offset += sound._delay;
		insertNewSound(offset, sound._note);
	}
	setPianorollLength(std::max((offset + kPianorollStride) / kPianorollStride * kPianorollStride, viewSize.width() / static_cast<size_t>(kStepWidth) + 1));
	const auto heightDifference = std::lround(sceneRect().height() - viewSize.height());
	if (heightDifference <= 0 || _soundItems.empty())
		return 0.5;
	auto rect = _soundItems.cbegin()->second->sceneBoundingRect();
	std::for_each(std::next(_soundItems.cbegin()), _soundItems.cend(), [&rect](const auto& soundItem) { rect |= soundItem.second->sceneBoundingRect(); });
	return (rect.center().y() - viewSize.height() / 2) / heightDifference;
}

void SequenceScene::setSequenceEditable(bool editable)
{
	_editable = editable;
}

void SequenceScene::insertNewSound(size_t offset, aulos::Note note)
{
	const auto [i, inserted] = _soundItems.emplace(offset, std::make_unique<SoundItem>(offset, note, _pianorollItem.get()));
	assert(inserted);
	const auto soundItem = i->second.get();
	connect(soundItem, &SoundItem::playRequested, [this, soundItem] { emit noteActivated(soundItem->note()); });
	connect(soundItem, &SoundItem::removeRequested, [this, offset] {
		if (_editable)
			removeSound(offset);
		else
			emit removingSound(offset);
	});
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
	setSceneRect({ 0, 0, kWhiteKeyWidth + steps * kStepWidth, _pianorollItem->boundingRect().height() });
	_pianorollItem->setStepCount(steps);
	_rightBoundItem->setPos(_pianorollItem->boundingRect().topRight());
}
