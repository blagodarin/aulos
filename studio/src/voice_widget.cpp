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
#include <QDoubleSpinBox>
#include <QGridLayout>
#include <QGroupBox>

struct VoiceWidget::EnvelopePoint
{
	QCheckBox* _check = nullptr;
	QDoubleSpinBox* _delay = nullptr;
	QDoubleSpinBox* _value = nullptr;
};

VoiceWidget::VoiceWidget(QWidget* parent)
	: QWidget{ parent }
{
	const auto createOscillationWidgets = [this](QWidget* parent) {
		const auto layout = new QGridLayout{ parent };

		auto margins = layout->contentsMargins();
		margins.setBottom(0);
		margins.setLeft(0);
		margins.setRight(0);
		layout->setContentsMargins(margins);

		_typeCombo = new QComboBox{ parent };
		_typeCombo->addItem(tr("Linear"), static_cast<int>(aulos::Wave::Linear));
		_typeCombo->addItem(tr("Quadratic"), static_cast<int>(aulos::Wave::Quadratic));
		layout->addWidget(_typeCombo, 0, 0);
		connect(_typeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &VoiceWidget::updateVoice);

		_oscillationSpin = new QDoubleSpinBox{ parent };
		_oscillationSpin->setRange(0.0, 1.0);
		_oscillationSpin->setSingleStep(0.01);
		layout->addWidget(_oscillationSpin, 0, 1);
		connect(_oscillationSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &VoiceWidget::updateVoice);

		layout->addItem(new QSpacerItem{ 0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding }, 1, 0, 1, 2);
	};

	const auto createEnvelopeEditor = [this](QWidget* parent, std::vector<EnvelopePoint>& envelope, double minimum) {
		const auto layout = new QGridLayout{ parent };

		auto margins = layout->contentsMargins();
		margins.setBottom(0);
		margins.setLeft(0);
		margins.setRight(0);
		layout->setContentsMargins(margins);

		for (int i = 0; i < 5; ++i)
		{
			auto& point = envelope.emplace_back();

			point._check = new QCheckBox{ tr("Point %1").arg(i), parent };
			point._check->setChecked(i == 0);
			point._check->setEnabled(i == 1);
			layout->addWidget(point._check, i, 0);
			connect(point._check, &QCheckBox::toggled, this, &VoiceWidget::updateVoice);

			point._delay = new QDoubleSpinBox{ parent };
			point._delay->setDecimals(2);
			point._delay->setEnabled(false);
			point._delay->setMaximum(60.0);
			point._delay->setMinimum(0.0);
			point._delay->setSingleStep(0.01);
			point._delay->setValue(0.0);
			layout->addWidget(point._delay, i, 1);
			connect(point._delay, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &VoiceWidget::updateVoice);

			point._value = new QDoubleSpinBox{ parent };
			point._value->setDecimals(2);
			point._value->setEnabled(i == 0);
			point._value->setMaximum(1.0);
			point._value->setMinimum(minimum);
			point._value->setSingleStep(0.01);
			point._value->setValue(1.0);
			layout->addWidget(point._value, i, 2);
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

	const auto layout = new QGridLayout{ this };

	const auto oscillationGroup = new QGroupBox{ tr("Oscillation"), this };
	oscillationGroup->setFlat(true);
	createOscillationWidgets(oscillationGroup);
	layout->addWidget(oscillationGroup, 0, 0);

	const auto amplitudeGroup = new QGroupBox{ tr("Amplitude"), this };
	amplitudeGroup->setFlat(true);
	createEnvelopeEditor(amplitudeGroup, _amplitudeEnvelope, 0.0);
	layout->addWidget(amplitudeGroup, 0, 1);

	const auto frequencyGroup = new QGroupBox{ tr("Frequency"), this };
	frequencyGroup->setFlat(true);
	createEnvelopeEditor(frequencyGroup, _frequencyEnvelope, 0.5);
	layout->addWidget(frequencyGroup, 1, 0);

	const auto asymmetryGroup = new QGroupBox{ tr("Asymmetry"), this };
	asymmetryGroup->setFlat(true);
	createEnvelopeEditor(asymmetryGroup, _asymmetryEnvelope, 0.0);
	layout->addWidget(asymmetryGroup, 1, 1);

	layout->addItem(new QSpacerItem{ 0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding }, 2, 0, 1, 2);
	layout->setRowStretch(2, 1);
}

void VoiceWidget::setVoice(const std::shared_ptr<aulos::Voice>& voice)
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
	aulos::Voice defaultVoice;
	const auto usedVoice = voice ? voice.get() : &defaultVoice;
	_typeCombo->setCurrentIndex(_typeCombo->findData(static_cast<int>(usedVoice->_wave)));
	_oscillationSpin->setValue(usedVoice->_oscillation);
	setEnvelope(_amplitudeEnvelope, usedVoice->_amplitudeEnvelope);
	setEnvelope(_frequencyEnvelope, usedVoice->_frequencyEnvelope);
	setEnvelope(_asymmetryEnvelope, usedVoice->_asymmetryEnvelope);
	_voice = voice;
}

void VoiceWidget::updateVoice()
{
	if (!_voice)
		return;
	_voice->_wave = static_cast<aulos::Wave>(_typeCombo->currentData().toInt());
	_voice->_oscillation = static_cast<float>(_oscillationSpin->value());
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
	emit voiceChanged();
}
