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

#include "sequence_widget.hpp"

#include "sequence_scene.hpp"

#include <QGraphicsView>
#include <QGridLayout>
#include <QScrollBar>

SequenceWidget::SequenceWidget(QWidget* parent)
	: QWidget{ parent }
	, _scene{ new SequenceScene{ this } }
{
	const auto layout = new QGridLayout{ this };
	layout->setContentsMargins({});

	_view = new QGraphicsView{ _scene, this };
	_view->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
	layout->addWidget(_view, 0, 0);

	connect(_scene, &SequenceScene::insertingSound, [this](size_t offset, aulos::Note note) {
		if (!_sequenceData)
			return;
		size_t position = 0;
		const auto sound = std::find_if(_sequenceData->_sounds.begin(), _sequenceData->_sounds.end(), [offset, &position](const aulos::Sound& sound) mutable {
			position += sound._delay;
			return position >= offset;
		});
		if (sound == _sequenceData->_sounds.end())
		{
			assert(position < offset || position == offset && _sequenceData->_sounds.empty());
			_sequenceData->_sounds.emplace_back(offset - position, note);
		}
		else if (position > offset)
		{
			assert(position > offset);
			const auto nextDelay = position - offset;
			assert(sound->_delay > nextDelay || sound->_delay == nextDelay && sound == _sequenceData->_sounds.begin());
			const auto delay = sound->_delay - nextDelay;
			sound->_delay = nextDelay;
			_sequenceData->_sounds.emplace(sound, delay, note);
		}
		else
		{
			assert(position == offset);
			sound->_note = note;
		}
		_scene->insertSound(offset, note);
		emit sequenceChanged();
	});
	connect(_scene, &SequenceScene::noteActivated, this, &SequenceWidget::noteActivated);
	connect(_scene, &SequenceScene::removingSound, [this](size_t offset) {
		if (!_sequenceData)
			return;
		const auto sound = std::find_if(_sequenceData->_sounds.begin(), _sequenceData->_sounds.end(), [offset, position = size_t{}](const aulos::Sound& sound) mutable {
			position += sound._delay;
			assert(position <= offset);
			return position == offset;
		});
		assert(sound != _sequenceData->_sounds.end());
		if (const auto nextSound = std::next(sound); nextSound != _sequenceData->_sounds.end())
			nextSound->_delay += sound->_delay;
		_sequenceData->_sounds.erase(sound);
		_scene->removeSound(offset);
		emit sequenceChanged();
	});
}

void SequenceWidget::setInteractive(bool interactive)
{
	_view->setInteractive(interactive);
}

void SequenceWidget::setSequence(const std::shared_ptr<aulos::SequenceData>& sequence)
{
	_sequenceData = sequence;
	const auto verticalPosition = _scene->setSequence(sequence ? *sequence : aulos::SequenceData{}, _view->size());
	const auto horizontalScrollBar = _view->horizontalScrollBar();
	horizontalScrollBar->setValue(horizontalScrollBar->minimum());
	const auto verticalScrollBar = _view->verticalScrollBar();
	verticalScrollBar->setValue(verticalScrollBar->minimum() + std::lround((verticalScrollBar->maximum() - verticalScrollBar->minimum()) * verticalPosition));
}
