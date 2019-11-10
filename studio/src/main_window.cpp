//
// This file is part of the Aulos toolkit.
// Copyright (C) 2019 Sergei Blagodarin.
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

#include "main_window.hpp"

#include <aulos.hpp>

#include <array>
#include <cassert>

#include <QAudioOutput>
#include <QGridLayout>
#include <QToolButton>

namespace
{
	const std::array<const char*, 12> noteNames{ "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
}

MainWindow::MainWindow()
	: _audioBuffer{ &_audioData }
{
	const auto layout = new QGridLayout{ this };
	for (int row = 0; row < 10; ++row)
		for (int column = 0; column < 12; ++column)
		{
			const auto name = noteNames[column];
			const auto button = new QToolButton{ this };
			button->setText(QStringLiteral("%1%2").arg(name).arg(9 - row));
			button->setFixedSize({ 40, 30 });
			button->setStyleSheet(name[1] == '#' ? "background-color: black; color: white" : "background-color: white; color: black");
			connect(button, &QAbstractButton::clicked, this, &MainWindow::onNoteClicked);
			layout->addWidget(button, row, column);
		}

	QAudioFormat format;
	format.setByteOrder(QAudioFormat::LittleEndian);
	format.setChannelCount(1);
	format.setCodec("audio/pcm");
	format.setSampleRate(48'000);
	format.setSampleSize(32);
	format.setSampleType(QAudioFormat::Float);
	_audioOutput = std::make_unique<QAudioOutput>(format);
}

MainWindow::~MainWindow() = default;

void MainWindow::onNoteClicked()
{
	const auto button = qobject_cast<QAbstractButton*>(sender());
	assert(button);

	QByteArray buffer;
	buffer += "wave 1 rectangle\n";
	buffer += "amplitude 1 adsr 0.01 0.04 0.5 0.95\n";
	buffer += "frequency 1 1.0\n";
	buffer += "asymmetry 1 0.0\n";
	buffer += "| " + button->text().toUtf8();

	const auto composition = aulos::Composition::create(buffer.constData());
	assert(composition);

	const auto renderer = aulos::Renderer::create(*composition, 48'000);
	assert(renderer);

	_audioOutput->stop();
	_audioBuffer.close();
	_audioData.clear();
	_audioData.resize(65'536);
	for (int totalRendered = 0;;)
		if (const auto rendered = renderer->render(_audioData.data() + totalRendered, _audioData.size() - totalRendered); rendered > 0)
		{
			totalRendered += static_cast<int>(rendered);
			_audioData.resize(totalRendered + 65'536);
		}
		else
		{
			_audioData.resize(totalRendered);
			break;
		}
	_audioBuffer.open(QIODevice::ReadOnly);
	_audioOutput->start(&_audioBuffer);
}
