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
class QSpinBox;

namespace aulos
{
	struct TrackProperties;
	struct VoiceData;
}

class VoiceWidget final : public QWidget
{
	Q_OBJECT

public:
	explicit VoiceWidget(QWidget* parent);
	~VoiceWidget() override;

	void setParameters(const std::shared_ptr<aulos::VoiceData>&, const std::shared_ptr<aulos::TrackProperties>&);
	std::shared_ptr<aulos::VoiceData> voice() const { return _voice; }

signals:
	void trackPropertiesChanged();
	void voiceChanged();

private:
	void updateShapeParameter();
	void updateTrackProperties();
	void updateVoice();
	void updateWaveImage();

private:
	struct EnvelopeChange;

	QSpinBox* _trackWeightSpin = nullptr;
	QComboBox* _waveShapeCombo = nullptr;
	QLabel* _waveShapeParameterLabel = nullptr;
	QDoubleSpinBox* _waveShapeParameterSpin = nullptr;
	QLabel* _waveShapeImage = nullptr;
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
	std::shared_ptr<aulos::TrackProperties> _trackProperties;
};
