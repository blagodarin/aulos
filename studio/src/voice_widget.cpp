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
	QDoubleSpinBox* _delay = nullptr;
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

	_typeCombo = new QComboBox{ this };
	_typeCombo->addItem(tr("Linear wave"), static_cast<int>(aulos::Wave::Linear));
	_typeCombo->addItem(tr("Quadratic wave"), static_cast<int>(aulos::Wave::Quadratic));
	_typeCombo->addItem(tr("Cubic wave"), static_cast<int>(aulos::Wave::Cubic));
	_typeCombo->addItem(tr("Cosine wave"), static_cast<int>(aulos::Wave::Cosine));
	layout->addItem(new QSpacerItem{ 0, 0, QSizePolicy::Fixed, QSizePolicy::Fixed }, row, 0);
	layout->addWidget(_typeCombo, row, 1, 1, 3);
	connect(_typeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &VoiceWidget::updateVoice);
	++row;

	createHeader(tr("Stereo settings"));

	_phaseShiftSpin = new QSpinBox{ parent };
	_phaseShiftSpin->setMaximum(1'000);
	_phaseShiftSpin->setMinimum(0);
	_phaseShiftSpin->setSingleStep(1);
	_phaseShiftSpin->setSuffix(tr("ms"));
	_phaseShiftSpin->setValue(0);
	layout->addWidget(new QLabel{ tr("Phase shift:"), this }, row, 1);
	layout->addWidget(_phaseShiftSpin, row, 2, 1, 2);
	connect(_phaseShiftSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &VoiceWidget::updateVoice);
	++row;

	_antiphaseCheck = new QCheckBox{ tr("Antiphase"), this };
	layout->addWidget(_antiphaseCheck, row, 1, 1, 3);
	connect(_antiphaseCheck, &QCheckBox::toggled, this, &VoiceWidget::updateVoice);
	++row;

	_panSpin = new QDoubleSpinBox{ parent };
	_panSpin->setDecimals(2);
	_panSpin->setMaximum(1.0);
	_panSpin->setMinimum(-1.0);
	_panSpin->setSingleStep(0.01);
	_panSpin->setValue(0.0);
	layout->addWidget(new QLabel{ tr("Linear pan:"), this }, row, 1);
	layout->addWidget(_panSpin, row, 2, 1, 2);
	connect(_panSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &VoiceWidget::updateVoice);
	++row;

	const auto createEnvelopeWidgets = [this, layout, &row](std::vector<EnvelopePoint>& envelope, double minimum) {
		for (int i = 0; i < 5; ++i, ++row)
		{
			auto& point = envelope.emplace_back();

			point._check = new QCheckBox{ tr("Point %1").arg(i), this };
			point._check->setChecked(i == 0);
			point._check->setEnabled(i == 1);
			layout->addWidget(point._check, row, 1);
			connect(point._check, &QCheckBox::toggled, this, &VoiceWidget::updateVoice);

			point._delay = new QDoubleSpinBox{ this };
			point._delay->setDecimals(2);
			point._delay->setEnabled(false);
			point._delay->setMaximum(60.0);
			point._delay->setMinimum(0.0);
			point._delay->setSingleStep(0.01);
			point._delay->setValue(0.0);
			layout->addWidget(point._delay, row, 2);
			connect(point._delay, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &VoiceWidget::updateVoice);

			point._value = new QDoubleSpinBox{ this };
			point._value->setDecimals(2);
			point._value->setEnabled(i == 0);
			point._value->setMaximum(1.0);
			point._value->setMinimum(minimum);
			point._value->setSingleStep(0.01);
			point._value->setValue(1.0);
			layout->addWidget(point._value, row, 3);
			connect(point._value, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &VoiceWidget::updateVoice);

			if (i == 0)
			{
				point._delay->setButtonSymbols(QAbstractSpinBox::NoButtons);
				point._delay->setFrame(false);
				point._delay->setStyleSheet("background: transparent");
			}
			else
			{
				const auto previousCheck = envelope[i - 1]._check;
				connect(previousCheck, &QCheckBox::toggled, point._check, &QWidget::setEnabled);
				connect(point._check, &QCheckBox::toggled, point._delay, &QWidget::setEnabled);
				if (i > 1)
					connect(point._check, &QCheckBox::toggled, previousCheck, &QWidget::setDisabled);
			}
			connect(point._check, &QCheckBox::toggled, point._value, &QWidget::setEnabled);
		}
	};

	createHeader(tr("Amplitude modulation"));
	createEnvelopeWidgets(_amplitudeEnvelope, 0.0);

	createHeader(tr("Frequency modulation"));
	createEnvelopeWidgets(_frequencyEnvelope, 0.5);

	createHeader(tr("Asymmetry modulation"));
	createEnvelopeWidgets(_asymmetryEnvelope, 0.0);

	createHeader(tr("Oscillation modulation"));
	createEnvelopeWidgets(_oscillationEnvelope, 0.0);

	layout->addItem(new QSpacerItem{ 0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding }, row, 0, 1, 4);
	layout->setRowStretch(row, 1);
}

VoiceWidget::~VoiceWidget() = default;

void VoiceWidget::setVoice(const std::shared_ptr<aulos::VoiceData>& voice)
{
	const auto setEnvelope = [](std::vector<EnvelopePoint>& dst, const aulos::Envelope& src) {
		assert(dst[0]._check->isChecked());
		dst[0]._delay->setValue(0.0);
		dst[0]._value->setValue(src._initial);
		for (size_t i = 1; i < dst.size(); ++i)
		{
			auto& dstPoint = dst[i];
			const auto use = i <= src._changes.size();
			dstPoint._check->setChecked(use);
			if (use)
			{
				auto& srcPoint = src._changes[i - 1];
				dstPoint._delay->setValue(srcPoint._delay);
				dstPoint._value->setValue(srcPoint._value);
			}
			else
			{
				dstPoint._delay->setValue(0.0);
				dstPoint._value->setValue(0.0);
			}
		}
	};

	_voice.reset(); // Prevent handling voice changes.
	aulos::VoiceData defaultVoice;
	const auto usedVoice = voice ? voice.get() : &defaultVoice;
	_typeCombo->setCurrentIndex(_typeCombo->findData(static_cast<int>(usedVoice->_wave)));
	_phaseShiftSpin->setValue(usedVoice->_phaseShift);
	_antiphaseCheck->setChecked(usedVoice->_antiphase);
	_panSpin->setValue(usedVoice->_pan);
	setEnvelope(_amplitudeEnvelope, usedVoice->_amplitudeEnvelope);
	setEnvelope(_frequencyEnvelope, usedVoice->_frequencyEnvelope);
	setEnvelope(_asymmetryEnvelope, usedVoice->_asymmetryEnvelope);
	setEnvelope(_oscillationEnvelope, usedVoice->_oscillationEnvelope);
	_voice = voice;
}

void VoiceWidget::updateVoice()
{
	if (!_voice)
		return;
	_voice->_wave = static_cast<aulos::Wave>(_typeCombo->currentData().toInt());
	_voice->_phaseShift = _phaseShiftSpin->value();
	_voice->_antiphase = _antiphaseCheck->isChecked();
	_voice->_pan = static_cast<float>(_panSpin->value());
	if (auto i = _amplitudeEnvelope.begin(); i->_check->isChecked())
	{
		_voice->_amplitudeEnvelope._initial = static_cast<float>(i->_value->value());
		_voice->_amplitudeEnvelope._changes.clear();
		for (++i; i != _amplitudeEnvelope.end() && i->_check->isChecked(); ++i)
			_voice->_amplitudeEnvelope._changes.emplace_back(static_cast<float>(i->_delay->value()), static_cast<float>(i->_value->value()));
	}
	if (auto i = _frequencyEnvelope.begin(); i->_check->isChecked())
	{
		_voice->_frequencyEnvelope._initial = static_cast<float>(i->_value->value());
		_voice->_frequencyEnvelope._changes.clear();
		for (++i; i != _frequencyEnvelope.end() && i->_check->isChecked(); ++i)
			_voice->_frequencyEnvelope._changes.emplace_back(static_cast<float>(i->_delay->value()), static_cast<float>(i->_value->value()));
	}
	if (auto i = _asymmetryEnvelope.begin(); i->_check->isChecked())
	{
		_voice->_asymmetryEnvelope._initial = static_cast<float>(i->_value->value());
		_voice->_asymmetryEnvelope._changes.clear();
		for (++i; i != _asymmetryEnvelope.end() && i->_check->isChecked(); ++i)
			_voice->_asymmetryEnvelope._changes.emplace_back(static_cast<float>(i->_delay->value()), static_cast<float>(i->_value->value()));
	}
	if (auto i = _oscillationEnvelope.begin(); i->_check->isChecked())
	{
		_voice->_oscillationEnvelope._initial = static_cast<float>(i->_value->value());
		_voice->_oscillationEnvelope._changes.clear();
		for (++i; i != _oscillationEnvelope.end() && i->_check->isChecked(); ++i)
			_voice->_oscillationEnvelope._changes.emplace_back(static_cast<float>(i->_delay->value()), static_cast<float>(i->_value->value()));
	}
	emit voiceChanged();
}
