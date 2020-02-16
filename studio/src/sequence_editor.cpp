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

#include "sequence_editor.hpp"

#include "player.hpp"
#include "sequence_scene.hpp"

#include <aulos/data.hpp>

#include <QDialogButtonBox>
#include <QGraphicsView>
#include <QGridLayout>
#include <QLabel>
#include <QScrollBar>

SequenceEditor::SequenceEditor(QWidget* parent)
	: QDialog{ parent, Qt::WindowTitleHint | Qt::CustomizeWindowHint | Qt::WindowCloseButtonHint }
	, _voice{ std::make_unique<aulos::Voice>() }
	, _sequence{ std::make_unique<aulos::SequenceData>() }
	, _scene{ new SequenceScene{ this } }
	, _player{ std::make_unique<Player>() }
{
	setWindowTitle(tr("Sequence Editor"));

	const auto rootLayout = new QGridLayout{ this };

	_sequenceView = new QGraphicsView{ _scene, this };
	rootLayout->addWidget(_sequenceView, 0, 0);

	const auto buttonBox = new QDialogButtonBox{ QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this };
	rootLayout->addWidget(buttonBox, 1, 0);
	connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
	connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

	connect(this, &QDialog::finished, [this] { _player->stop(); });

	connect(_scene, &SequenceScene::noteActivated, [this](aulos::Note note) {
		const auto renderer = aulos::VoiceRenderer::create(*_voice, Player::SamplingRate);
		assert(renderer);
		renderer->start(note, 1.f);
		_player->reset(*renderer);
		_player->start();
	});
}

SequenceEditor::~SequenceEditor() = default;

void SequenceEditor::setSequence(const aulos::Voice& voice, const aulos::SequenceData& sequence)
{
	*_voice = voice;
	*_sequence = sequence;
	_scene->setSequence(sequence);
	const auto horizontalScrollBar = _sequenceView->horizontalScrollBar();
	horizontalScrollBar->setValue(horizontalScrollBar->minimum());
	const auto verticalScrollBar = _sequenceView->verticalScrollBar();
	verticalScrollBar->setValue((verticalScrollBar->minimum() + verticalScrollBar->maximum()) / 2);
}

aulos::SequenceData SequenceEditor::sequence() const
{
	return *_sequence;
}
