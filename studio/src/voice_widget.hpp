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

#include <memory>

#include <QWidget>

class QDoubleSpinBox;

namespace aulos
{
	struct Voice;
}

class VoiceWidget final : public QWidget
{
	Q_OBJECT

public:
	VoiceWidget(QWidget* parent);

	void setVoice(const std::shared_ptr<aulos::Voice>&);
	std::shared_ptr<aulos::Voice> voice() const { return _voice; }

signals:
	void voiceChanged();

private:
	void updateVoice();

private:
	struct EnvelopePoint;

	QDoubleSpinBox* _oscillationSpin = nullptr;
	std::vector<EnvelopePoint> _amplitudeEnvelope;
	std::vector<EnvelopePoint> _frequencyEnvelope;
	std::vector<EnvelopePoint> _asymmetryEnvelope;
	std::shared_ptr<aulos::Voice> _voice;
};
