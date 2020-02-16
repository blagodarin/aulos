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
#include "key_item.hpp"
#include "pianoroll_item.hpp"
#include "sound_item.hpp"
#include "utils.hpp"

SequenceScene::SequenceScene(QObject* parent)
	: QGraphicsScene{ parent }
{
	setBackgroundBrush(kBackgroundColor);
	for (int i = 0; i < 120; ++i)
	{
		const auto note = static_cast<aulos::Note>(i);
		const auto keyItem = new KeyItem{ note };
		addItem(keyItem);
		connect(keyItem, &KeyItem::activated, this, [this, note] { emit noteActivated(note); });
	}
	_pianorollItem = std::make_unique<PianorollItem>();
	addItem(_pianorollItem.get());
}

SequenceScene::~SequenceScene()
{
	removeSoundItems();
	removeItem(_pianorollItem.get());
}

void SequenceScene::addPianorollSteps()
{
	_pianorollItem->setStepCount(_pianorollItem->stepCount() + 8);
	setSceneRect(_pianorollItem->boundingRect().adjusted(-kWhiteKeyWidth, 0, 0, 0));
}

void SequenceScene::setSequence(const aulos::SequenceData& sequence)
{
	removeSoundItems();
	size_t offset = 0;
	for (const auto& sound : sequence._sounds)
	{
		const auto& soundItem = _soundItems.emplace_back(std::make_unique<SoundItem>(_pianorollItem.get()));
		soundItem->setPos(offset * kStepWidth, (119 - static_cast<size_t>(sound._note)) * kNoteHeight);
		offset += sound._pause;
	}
	_pianorollItem->setStepCount((offset + 8) / 8 * 8);
	setSceneRect(_pianorollItem->boundingRect().adjusted(-kWhiteKeyWidth, 0, 0, 0));
}

void SequenceScene::removeSoundItems()
{
	// Qt documentation says it's more efficient to remove items from the scene before destroying them.
	for (const auto& sound : _soundItems)
		removeItem(sound.get());
	_soundItems.clear();
}
