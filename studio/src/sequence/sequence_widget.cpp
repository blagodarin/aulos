// This file is part of the Aulos toolkit.
// Copyright (C) Sergei Blagodarin.
// SPDX-License-Identifier: Apache-2.0

#include "sequence_widget.hpp"

#include "sequence_scene.hpp"

#include <cassert>
#include <cmath>

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

	connect(_scene, &SequenceScene::insertingSound, [this](size_t offset, seir::synth::Note note) {
		if (!_sequenceData)
			return;
		size_t position = 0;
		const auto first = std::find_if(_sequenceData->_sounds.begin(), _sequenceData->_sounds.end(), [offset, &position](const seir::synth::Sound& sound) {
			position += sound._delay;
			return position >= offset;
		});
		if (first == _sequenceData->_sounds.end())
		{
			assert(position < offset || (position == offset && _sequenceData->_sounds.empty()));
			_sequenceData->_sounds.emplace_back(offset - position, note);
		}
		else if (position > offset)
		{
			assert(position > offset);
			const auto nextDelay = position - offset;
			assert(first->_delay > nextDelay || (first->_delay == nextDelay && first == _sequenceData->_sounds.begin()));
			const auto delay = first->_delay - nextDelay;
			first->_delay = nextDelay;
			_sequenceData->_sounds.emplace(first, delay, note);
		}
		else
		{
			assert(position == offset);
			const auto end = std::find_if(std::next(first), _sequenceData->_sounds.end(), [](const seir::synth::Sound& sound) { return sound._delay > 0; });
			const auto before = std::find_if(first, end, [note](const seir::synth::Sound& sound) { return sound._note <= note; });
			assert(before == end || before->_note != note);
			const auto delay = before == first ? std::exchange(first->_delay, 0) : 0;
			_sequenceData->_sounds.emplace(before, delay, note);
		}
		_scene->insertSound(offset, note);
		emit sequenceChanged();
	});
	connect(_scene, &SequenceScene::noteActivated, this, &SequenceWidget::noteActivated);
	connect(_scene, &SequenceScene::removingSound, [this](size_t offset, seir::synth::Note note) {
		if (!_sequenceData)
			return;
		const auto sound = std::find_if(_sequenceData->_sounds.begin(), _sequenceData->_sounds.end(), [offset, note, position = size_t{}](const seir::synth::Sound& sound) mutable {
			position += sound._delay;
			assert(position <= offset);
			return position == offset && sound._note == note;
		});
		assert(sound != _sequenceData->_sounds.end());
		if (const auto nextSound = std::next(sound); nextSound != _sequenceData->_sounds.end())
			nextSound->_delay += sound->_delay;
		_sequenceData->_sounds.erase(sound);
		_scene->removeSound(offset, note);
		emit sequenceChanged();
	});
}

void SequenceWidget::setInteractive(bool interactive)
{
	_view->setInteractive(interactive);
}

void SequenceWidget::setSequence(const std::shared_ptr<seir::synth::SequenceData>& sequence)
{
	_sequenceData = sequence;
	const auto verticalPosition = _scene->setSequence(sequence ? *sequence : seir::synth::SequenceData{}, _view->size());
	const auto horizontalScrollBar = _view->horizontalScrollBar();
	horizontalScrollBar->setValue(horizontalScrollBar->minimum());
	const auto verticalScrollBar = _view->verticalScrollBar();
	verticalScrollBar->setValue(verticalScrollBar->minimum() + std::lround((verticalScrollBar->maximum() - verticalScrollBar->minimum()) * verticalPosition));
}
