// This file is part of the Aulos toolkit.
// Copyright (C) Sergei Blagodarin.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <memory>

#include <QScrollArea>

class QCheckBox;
class QComboBox;
class QDoubleSpinBox;
class QGroupBox;
class QSpinBox;

namespace aulos
{
	struct TrackProperties;
	struct VoiceData;
}

class WaveShapeWidget;

class VoiceWidget final : public QScrollArea
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

	QGroupBox* _trackGroup = nullptr;
	QSpinBox* _trackWeightSpin = nullptr;
	QComboBox* _polyphonyCombo = nullptr;
	QDoubleSpinBox* _headDelaySpin = nullptr;
	QDoubleSpinBox* _sourceDistanceSpin = nullptr;
	QSpinBox* _sourceWidthSpin = nullptr;
	QSpinBox* _sourceOffsetSpin = nullptr;
	QDoubleSpinBox* _stereoPanSpin = nullptr;
	QCheckBox* _stereoInversionCheck = nullptr;
	QComboBox* _waveShapeCombo = nullptr;
	QDoubleSpinBox* _waveShapeParameterSpin = nullptr;
	WaveShapeWidget* _waveShapeWidget = nullptr;
	std::vector<EnvelopeChange> _amplitudeEnvelope;
	std::vector<EnvelopeChange> _frequencyEnvelope;
	std::vector<EnvelopeChange> _asymmetryEnvelope;
	std::vector<EnvelopeChange> _oscillationEnvelope;
	std::shared_ptr<aulos::VoiceData> _voice;
	std::shared_ptr<aulos::TrackProperties> _trackProperties;
};
