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

#include "voice_editor.hpp"

#include "player.hpp"
#include "voice_widget.hpp"

#include <aulos/data.hpp>

#include <array>
#include <cassert>

#include <QDialogButtonBox>
#include <QGridLayout>
#include <QToolButton>
#include <QVariant>

namespace
{
	const std::array<const char*, 12> noteNames{ "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
}

VoiceEditor::VoiceEditor(QWidget* parent)
	: QDialog{ parent, Qt::WindowTitleHint | Qt::CustomizeWindowHint | Qt::WindowCloseButtonHint }
	, _player{ std::make_unique<Player>() }
{
	setWindowTitle(tr("Voice Editor"));

	const auto rootLayout = new QGridLayout{ this };

	_voiceWidget = new VoiceWidget{ this };
	rootLayout->addWidget(_voiceWidget, 0, 0);

	const auto noteLayout = new QGridLayout{};
	rootLayout->addLayout(noteLayout, 0, 1);
	for (int row = 0; row < 10; ++row)
		for (int column = 0; column < 12; ++column)
		{
			const auto octave = 9 - row;
			const auto name = noteNames[column];
			const auto button = new QToolButton{ this };
			button->setText(QStringLiteral("%1%2").arg(name).arg(octave));
			button->setFixedSize({ 40, 30 });
			button->setStyleSheet(name[1] == '#' ? "background-color: black; color: white" : "background-color: white; color: black");
			button->setProperty("note", octave * 12 + column);
			connect(button, &QAbstractButton::clicked, this, &VoiceEditor::onNoteClicked);
			noteLayout->addWidget(button, row, column);
		}

	const auto buttonBox = new QDialogButtonBox{ QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this };
	rootLayout->addWidget(buttonBox, 1, 0, 1, 2);
	connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
	connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

	connect(this, &QDialog::finished, [this] { _player->stop(); });
}

void VoiceEditor::setVoice(const aulos::Voice& voice)
{
	_voiceWidget->setVoice(voice);
}

aulos::Voice VoiceEditor::voice() const
{
	return _voiceWidget->voice();
}

void VoiceEditor::onNoteClicked()
{
	const auto renderer = aulos::VoiceRenderer::create(voice(), Player::SamplingRate);
	assert(renderer);

	const auto button = qobject_cast<QAbstractButton*>(sender());
	assert(button);
	renderer->start(static_cast<aulos::Note>(button->property("note").toInt()), 1.f);

	_player->reset(*renderer);
	_player->start();
}
