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

#pragma once

#include <memory>

#include <QByteArray>
#include <QBuffer>
#include <QWidget>

class QAudioOutput;

class Ui_MainWindow;

class MainWindow : public QWidget
{
	Q_OBJECT

public:
	MainWindow();
	~MainWindow() override;

private slots:
	void onNoteClicked();

private:
	struct EnvelopePoint;

	void createEnvelopeEditor(QWidget*, std::vector<EnvelopePoint>&, double minimum);

private:
	const std::unique_ptr<Ui_MainWindow> _ui;
	std::vector<EnvelopePoint> _frequencyEnvelope;
	std::vector<EnvelopePoint> _asymmetryEnvelope;
	QByteArray _audioData;
	QBuffer _audioBuffer;
	std::unique_ptr<QAudioOutput> _audioOutput;
};