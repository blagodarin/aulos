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

#include "voice_widget.hpp"

#include <aulos/data.hpp>

#include <cassert>

#include <QCheckBox>
#include <QComboBox>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QSpinBox>

struct VoiceWidget::EnvelopePoint
{
	QCheckBox* _check = nullptr;
	QSpinBox* _delay = nullptr;
	QDoubleSpinBox* _value = nullptr;
};

VoiceWidget::VoiceWidget(QWidget* parent)
	: QWidget{ parent }
{
	const auto layout = new QGridLayout{ this };
	int row = 0;

	const auto createHeader = [this, layout, &row](const QString& title) {
		const auto header = new QLabel{ title, this };
		header->setStyleSheet("font-weight: bold");
		layout->addWidget(header, row, 0, 1, 4);
		++row;
	};

	_waveShapeCombo = new QComboBox{ this };
	_waveShapeCombo->addItem(tr("Linear"), static_cast<int>(aulos::WaveShape::Linear));
	_waveShapeCombo->addItem(tr("Quadratic 1"), static_cast<int>(aulos::WaveShape::Quadratic1));
	_waveShapeCombo->addItem(tr("Quadratic 2"), static_cast<int>(aulos::WaveShape::Quadratic2));
	_waveShapeCombo->addItem(tr("Cubic"), static_cast<int>(aulos::WaveShape::Cubic));
	_waveShapeCombo->addItem(tr("Quintic"), static_cast<int>(aulos::WaveShape::Quintic));
	_waveShapeCombo->addItem(tr("Cosine"), static_cast<int>(aulos::WaveShape::Cosine));
	layout->addItem(new QSpacerItem{ 0, 0, QSizePolicy::Fixed, QSizePolicy::Fixed }, row, 0);
	layout->addWidget(new QLabel{ tr("Wave shape:"), this }, row, 1);
	layout->addWidget(_waveShapeCombo, row, 2, 1, 2);
	connect(_waveShapeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &VoiceWidget::updateVoice);
	++row;

	createHeader(tr("Stereo parameters"));

	_stereoDelaySpin = new QDoubleSpinBox{ parent };
	_stereoDelaySpin->setDecimals(2);
	_stereoDelaySpin->setMaximum(1'000.0);
	_stereoDelaySpin->setMinimum(-1'000.0);
	_stereoDelaySpin->setSingleStep(0.01);
	_stereoDelaySpin->setSuffix(tr("ms"));
	_stereoDelaySpin->setValue(0.0);
	layout->addWidget(new QLabel{ tr("Delay:"), this }, row, 1);
	layout->addWidget(_stereoDelaySpin, row, 2, 1, 2);
	connect(_stereoDelaySpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &VoiceWidget::updateVoice);
	++row;

	_stereoPanSpin = new QDoubleSpinBox{ parent };
	_stereoPanSpin->setDecimals(2);
	_stereoPanSpin->setMaximum(1.0);
	_stereoPanSpin->setMinimum(-1.0);
	_stereoPanSpin->setSingleStep(0.01);
	_stereoPanSpin->setValue(0.0);
	layout->addWidget(new QLabel{ tr("Pan:"), this }, row, 1);
	layout->addWidget(_stereoPanSpin, row, 2, 1, 2);
	connect(_stereoPanSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &VoiceWidget::updateVoice);
	++row;

	_stereoInversionCheck = new QCheckBox{ tr("Invert right channel"), this };
	layout->addWidget(_stereoInversionCheck, row, 1, 1, 3);
	connect(_stereoInversionCheck, &QCheckBox::toggled, this, &VoiceWidget::updateVoice);
	++row;

	const auto createEnvelopeWidgets = [this, layout, &row](std::vector<EnvelopePoint>& envelope, double minimum) {
		for (int i = 0; i < 5; ++i, ++row)
		{
			auto& point = envelope.emplace_back();

			point._check = new QCheckBox{ tr("Point %1").arg(i), this };
			point._check->setChecked(false);
			point._check->setEnabled(i == 0);
			layout->addWidget(point._check, row, 1);
			connect(point._check, &QCheckBox::toggled, this, &VoiceWidget::updateVoice);

			point._delay = new QSpinBox{ this };
			point._delay->setEnabled(false);
			point._delay->setMaximum(aulos::Point::kMaxDelayMs);
			point._delay->setMinimum(0);
			point._delay->setSingleStep(1);
			point._delay->setSuffix(tr("ms"));
			point._delay->setValue(0);
			layout->addWidget(point._delay, row, 2);
			connect(point._delay, QOverload<int>::of(&QSpinBox::valueChanged), this, &VoiceWidget::updateVoice);

			point._value = new QDoubleSpinBox{ this };
			point._value->setDecimals(2);
			point._value->setEnabled(false);
			point._value->setMaximum(1.0);
			point._value->setMinimum(minimum);
			point._value->setSingleStep(0.01);
			point._value->setValue(0.0);
			layout->addWidget(point._value, row, 3);
			connect(point._value, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &VoiceWidget::updateVoice);

			connect(point._check, &QCheckBox::toggled, point._delay, &QWidget::setEnabled);
			connect(point._check, &QCheckBox::toggled, point._value, &QWidget::setEnabled);
			if (i > 0)
			{
				const auto previousCheck = envelope[i - 1]._check;
				connect(previousCheck, &QCheckBox::toggled, point._check, &QWidget::setEnabled);
				connect(point._check, &QCheckBox::toggled, previousCheck, &QWidget::setDisabled);
			}
		}
	};

	createHeader(tr("Amplitude modulation"));
	createEnvelopeWidgets(_amplitudeEnvelope, 0);

	createHeader(tr("Frequency modulation"));
	createEnvelopeWidgets(_frequencyEnvelope, -1);

	createHeader(tr("Asymmetry modulation"));
	createEnvelopeWidgets(_asymmetryEnvelope, 0);

	createHeader(tr("Oscillation modulation"));
	createEnvelopeWidgets(_oscillationEnvelope, 0);

	layout->addItem(new QSpacerItem{ 0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding }, row, 0, 1, 4);
	layout->setRowStretch(row, 1);
}

VoiceWidget::~VoiceWidget() = default;

void VoiceWidget::setVoice(const std::shared_ptr<aulos::VoiceData>& voice)
{
	const auto loadEnvelope = [](std::vector<EnvelopePoint>& dst, const aulos::Envelope& src) {
		for (auto i = dst.rbegin(); i != dst.rend(); ++i)
		{
			i->_check->setChecked(false);
			i->_delay->setValue(0);
			i->_value->setValue(0);
		}
		for (size_t i = 0; i < std::min(dst.size(), src._points.size()); ++i)
		{
			dst[i]._check->setChecked(true);
			dst[i]._delay->setValue(src._points[i]._delayMs);
			dst[i]._value->setValue(src._points[i]._value);
		}
	};

	_voice.reset(); // Prevent handling voice changes.
	aulos::VoiceData defaultVoice;
	const auto usedVoice = voice ? voice.get() : &defaultVoice;
	_waveShapeCombo->setCurrentIndex(_waveShapeCombo->findData(static_cast<int>(usedVoice->_waveShape)));
	_stereoDelaySpin->setValue(usedVoice->_stereoDelay);
	_stereoPanSpin->setValue(usedVoice->_stereoPan);
	_stereoInversionCheck->setChecked(usedVoice->_stereoInversion);
	loadEnvelope(_amplitudeEnvelope, usedVoice->_amplitudeEnvelope);
	loadEnvelope(_frequencyEnvelope, usedVoice->_frequencyEnvelope);
	loadEnvelope(_asymmetryEnvelope, usedVoice->_asymmetryEnvelope);
	loadEnvelope(_oscillationEnvelope, usedVoice->_oscillationEnvelope);
	_voice = voice;
}

void VoiceWidget::updateVoice()
{
	const auto storeEnvelope = [](aulos::Envelope& dst, const std::vector<EnvelopePoint>& src) {
		dst._points.clear();
		for (auto i = src.begin(); i != src.end() && i->_check->isChecked(); ++i)
			dst._points.emplace_back(i->_delay->value(), static_cast<float>(i->_value->value()));
	};

	if (!_voice)
		return;
	_voice->_waveShape = static_cast<aulos::WaveShape>(_waveShapeCombo->currentData().toInt());
	storeEnvelope(_voice->_amplitudeEnvelope, _amplitudeEnvelope);
	storeEnvelope(_voice->_frequencyEnvelope, _frequencyEnvelope);
	storeEnvelope(_voice->_asymmetryEnvelope, _asymmetryEnvelope);
	storeEnvelope(_voice->_oscillationEnvelope, _oscillationEnvelope);
	_voice->_stereoDelay = static_cast<float>(_stereoDelaySpin->value());
	_voice->_stereoPan = static_cast<float>(_stereoPanSpin->value());
	_voice->_stereoInversion = _stereoInversionCheck->isChecked();
	emit voiceChanged();
}
