// This file is part of the Aulos toolkit.
// Copyright (C) Sergei Blagodarin.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <memory>

#include <QWidget>

class QCheckBox;
class QComboBox;
class QDoubleSpinBox;
class QLabel;

namespace aulos
{
	struct VoiceData;
}

class VoiceWidget final : public QWidget
{
	Q_OBJECT

public:
	explicit VoiceWidget(QWidget* parent);
	~VoiceWidget() override;

	void setVoice(const std::shared_ptr<aulos::VoiceData>&);
	std::shared_ptr<aulos::VoiceData> voice() const { return _voice; }

signals:
	void voiceChanged();

private:
	void updateShapeParameter();
	void updateVoice();

private:
	struct EnvelopeChange;

	QComboBox* _waveShapeCombo = nullptr;
	QLabel* _waveShapeParameterLabel = nullptr;
	QDoubleSpinBox* _waveShapeParameterSpin = nullptr;
	QDoubleSpinBox* _stereoDelaySpin = nullptr;
	QDoubleSpinBox* _stereoRadiusSpin = nullptr;
	QDoubleSpinBox* _stereoPanSpin = nullptr;
	QCheckBox* _stereoInversionCheck = nullptr;
	QComboBox* _polyphonyCombo = nullptr;
	std::vector<EnvelopeChange> _amplitudeEnvelope;
	std::vector<EnvelopeChange> _frequencyEnvelope;
	std::vector<EnvelopeChange> _asymmetryEnvelope;
	std::vector<EnvelopeChange> _oscillationEnvelope;
	std::shared_ptr<aulos::VoiceData> _voice;
};
