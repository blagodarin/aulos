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

namespace seir::synth
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

	void setParameters(const std::shared_ptr<seir::synth::VoiceData>&, const std::shared_ptr<seir::synth::TrackProperties>&);
	std::shared_ptr<seir::synth::VoiceData> voice() const { return _voice; }

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
	QComboBox* _waveShapeCombo = nullptr;
	QDoubleSpinBox* _waveShapeParameterSpin1 = nullptr;
	QDoubleSpinBox* _waveShapeParameterSpin2 = nullptr;
	WaveShapeWidget* _waveShapeWidget = nullptr;
	std::vector<EnvelopeChange> _amplitudeEnvelope;
	QSpinBox* _tremoloFrequencySpin = nullptr;
	QDoubleSpinBox* _tremoloMagnitudeSpin = nullptr;
	std::vector<EnvelopeChange> _frequencyEnvelope;
	QSpinBox* _vibratoFrequencySpin = nullptr;
	QDoubleSpinBox* _vibratoMagnitudeSpin = nullptr;
	std::vector<EnvelopeChange> _asymmetryEnvelope;
	QSpinBox* _asymmetryFrequencySpin = nullptr;
	QDoubleSpinBox* _asymmetryMagnitudeSpin = nullptr;
	std::vector<EnvelopeChange> _rectangularityEnvelope;
	QSpinBox* _rectangularityFrequencySpin = nullptr;
	QDoubleSpinBox* _rectangularityMagnitudeSpin = nullptr;
	std::shared_ptr<seir::synth::VoiceData> _voice;
	std::shared_ptr<seir::synth::TrackProperties> _trackProperties;
};
