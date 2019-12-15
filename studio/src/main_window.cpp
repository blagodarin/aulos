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

#include "ui_main_window.h"

namespace
{
	const std::array<const char*, 12> noteNames{ "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
}

struct MainWindow::EnvelopePoint
{
	QCheckBox* _check = nullptr;
	QDoubleSpinBox* _delay = nullptr;
	QDoubleSpinBox* _value = nullptr;
};

MainWindow::MainWindow()
	: _ui{ std::make_unique<Ui_MainWindow>() }
	, _audioBuffer{ &_audioData }
{
	_ui->setupUi(this);

	createEnvelopeEditor(_ui->frequencyGroup, _frequencyEnvelope, 0.5);
	createEnvelopeEditor(_ui->asymmetryGroup, _asymmetryEnvelope, 0.0);

	const auto noteLayout = new QGridLayout{ _ui->noteWidget };
	for (int row = 0; row < 10; ++row)
		for (int column = 0; column < 12; ++column)
		{
			const auto name = noteNames[column];
			const auto button = new QToolButton{ this };
			button->setText(QStringLiteral("%1%2").arg(name).arg(9 - row));
			button->setFixedSize({ 40, 30 });
			button->setStyleSheet(name[1] == '#' ? "background-color: black; color: white" : "background-color: white; color: black");
			connect(button, &QAbstractButton::clicked, this, &MainWindow::onNoteClicked);
			noteLayout->addWidget(button, row, column);
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
	const auto printDouble = [](double value) {
		return QStringLiteral(" %1").arg(value, 0, 'f', 2).toUtf8();
	};

	const auto button = qobject_cast<QAbstractButton*>(sender());
	assert(button);

	QByteArray buffer;
	buffer += "@voice 1\n";
	buffer += "wave linear" + printDouble(_ui->oscillationSpin->value()) + "\n";
	buffer += "amplitude ";
	buffer += _ui->holdCheck->isChecked() ? "ahdsr" : "adsr";
	buffer += printDouble(_ui->attackSpin->value());
	if (_ui->holdCheck->isChecked())
		buffer += printDouble(_ui->holdSpin->value());
	buffer += printDouble(_ui->decaySpin->value());
	buffer += printDouble(_ui->sustainSpin->value());
	buffer += printDouble(_ui->releaseSpin->value());
	buffer += '\n';
	if (auto i = _frequencyEnvelope.begin(); i->_check->isChecked())
	{
		buffer += "frequency" + printDouble(i->_value->value());
		for (++i; i != _frequencyEnvelope.end() && i->_check->isChecked(); ++i)
		{
			buffer += printDouble(i->_delay->value());
			buffer += printDouble(i->_value->value());
		}
		buffer += '\n';
	}
	if (auto i = _asymmetryEnvelope.begin(); i->_check->isChecked())
	{
		buffer += "asymmetry" + printDouble(i->_value->value());
		for (++i; i != _asymmetryEnvelope.end() && i->_check->isChecked(); ++i)
		{
			buffer += printDouble(i->_delay->value());
			buffer += printDouble(i->_value->value());
		}
		buffer += '\n';
	}
	buffer += "@tracks\n";
	buffer += "1 1 1\n";
	buffer += "@sequences\n";
	buffer += "1 " + button->text().toUtf8() + "\n";
	buffer += "@fragments\n";
	buffer += "0 1 1\n";

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

void MainWindow::createEnvelopeEditor(QWidget* parent, std::vector<EnvelopePoint>& envelope, double minimum)
{
	const auto layout = new QGridLayout{ parent };
	layout->addWidget(new QLabel{ tr("Delay"), parent }, 0, 1);
	layout->addWidget(new QLabel{ tr("Value"), parent }, 0, 2);

	for (int i = 1; i <= 5; ++i)
	{
		auto& point = envelope.emplace_back();

		point._check = new QCheckBox{ tr("Point %1").arg(i), parent };
		point._check->setEnabled(i == 1);
		layout->addWidget(point._check, i, 0);

		point._delay = new QDoubleSpinBox{ parent };
		point._delay->setDecimals(2);
		point._delay->setEnabled(false);
		point._delay->setMaximum(60.0);
		point._delay->setMinimum(0.0);
		point._delay->setSingleStep(0.01);
		point._delay->setValue(0.0);
		layout->addWidget(point._delay, i, 1);

		point._value = new QDoubleSpinBox{ parent };
		point._value->setDecimals(2);
		point._value->setEnabled(false);
		point._value->setMaximum(1.0);
		point._value->setMinimum(minimum);
		point._value->setSingleStep(0.01);
		point._value->setValue(1.0);
		layout->addWidget(point._value, i, 2);

		if (i > 1)
		{
			const auto previousCheck = envelope[i - 2]._check;
			connect(previousCheck, &QCheckBox::toggled, point._check, &QWidget::setEnabled);
			connect(point._check, &QCheckBox::toggled, point._delay, &QWidget::setEnabled);
			connect(point._check, &QCheckBox::toggled, previousCheck, &QWidget::setDisabled);
		}
		connect(point._check, &QCheckBox::toggled, point._value, &QWidget::setEnabled);
	}
}
