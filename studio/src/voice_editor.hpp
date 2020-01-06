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

#pragma once

#include <vector>

#include <QByteArray>
#include <QBuffer>
#include <QWidget>

namespace aulos
{
	struct Voice;
}

class QAudioOutput;
class QDoubleSpinBox;
class QLineEdit;
class QTableView;

class VoicesModel;

class VoiceEditor : public QWidget
{
	Q_OBJECT

public:
	VoiceEditor(VoicesModel&, QWidget*);
	~VoiceEditor() override;

private slots:
	void onNoteClicked();

private:
	void hideEvent(QHideEvent*) override;

	void resetVoice(const aulos::Voice&);
	aulos::Voice currentVoice();

private:
	struct EnvelopePoint;

	VoicesModel& _model;
	QTableView* _voicesView;
	QDoubleSpinBox* _oscillationSpin;
	std::vector<EnvelopePoint> _amplitudeEnvelope;
	std::vector<EnvelopePoint> _frequencyEnvelope;
	std::vector<EnvelopePoint> _asymmetryEnvelope;
	QByteArray _audioData;
	QBuffer _audioBuffer;
	std::unique_ptr<QAudioOutput> _audioOutput;
};
