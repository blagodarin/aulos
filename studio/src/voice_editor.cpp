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

#include "voice_editor.hpp"

#include "voices_model.hpp"

#include <aulos.hpp>

#include <array>
#include <cassert>

#include <QAudioOutput>
#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QShowEvent>
#include <QStyledItemDelegate>
#include <QTableView>
#include <QToolButton>
#include <QVBoxLayout>

namespace
{
	const std::array<const char*, 12> noteNames{ "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };

	class VoiceDelegate : public QStyledItemDelegate
	{
	public:
		VoiceDelegate(QObject* parent)
			: QStyledItemDelegate{ parent } {}

		QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem&, const QModelIndex&) const override
		{
			const auto editor = new QLineEdit{ parent };
			editor->setMaxLength(64);
			editor->setValidator(new QRegExpValidator{ QRegExp{ "\\w+" }, editor });
			return editor;
		}

		void setEditorData(QWidget* editor, const QModelIndex& index) const override
		{
			qobject_cast<QLineEdit*>(editor)->setText(index.data(Qt::EditRole).toString());
		}

		void setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const override
		{
			model->setData(index, qobject_cast<QLineEdit*>(editor)->text(), Qt::EditRole);
		}
	};
}

struct VoiceEditor::EnvelopePoint
{
	QCheckBox* _check = nullptr;
	QDoubleSpinBox* _delay = nullptr;
	QDoubleSpinBox* _value = nullptr;
};

VoiceEditor::VoiceEditor(VoicesModel& model, QWidget* parent)
	: QWidget{ parent, Qt::Window | Qt::WindowTitleHint | Qt::CustomizeWindowHint | Qt::WindowCloseButtonHint }
	, _model{ model }
	, _audioBuffer{ &_audioData }
{
	QAudioFormat format;
	format.setByteOrder(QAudioFormat::LittleEndian);
	format.setChannelCount(1);
	format.setCodec("audio/pcm");
	format.setSampleRate(48'000);
	format.setSampleSize(32);
	format.setSampleType(QAudioFormat::Float);
	_audioOutput = std::make_unique<QAudioOutput>(format);

	const auto createOscillationWidgets = [this](QWidget* parent) {
		const auto layout = new QGridLayout{ parent };

		const auto typeCombo = new QComboBox{ parent };
		typeCombo->addItem(tr("Linear"));
		layout->addWidget(typeCombo, 0, 0);

		_oscillationSpin = new QDoubleSpinBox{ parent };
		_oscillationSpin->setRange(0.0, 1.0);
		_oscillationSpin->setSingleStep(0.01);
		layout->addWidget(_oscillationSpin, 0, 1);
	};

	const auto createEnvelopeEditor = [this](QWidget* parent, std::vector<EnvelopePoint>& envelope, double minimum) {
		const auto layout = new QGridLayout{ parent };
		layout->addWidget(new QLabel{ tr("Delay"), parent }, 0, 1);
		layout->addWidget(new QLabel{ tr("Value"), parent }, 0, 2);

		for (int i = 0; i < 5; ++i)
		{
			auto& point = envelope.emplace_back();

			point._check = new QCheckBox{ tr("Point %1").arg(i), parent };
			point._check->setChecked(i == 0);
			point._check->setEnabled(i == 1);
			layout->addWidget(point._check, i + 1, 0);

			point._delay = new QDoubleSpinBox{ parent };
			point._delay->setDecimals(2);
			point._delay->setEnabled(false);
			point._delay->setMaximum(60.0);
			point._delay->setMinimum(0.0);
			point._delay->setSingleStep(0.01);
			point._delay->setValue(0.0);
			layout->addWidget(point._delay, i + 1, 1);

			point._value = new QDoubleSpinBox{ parent };
			point._value->setDecimals(2);
			point._value->setEnabled(i == 0);
			point._value->setMaximum(1.0);
			point._value->setMinimum(minimum);
			point._value->setSingleStep(0.01);
			point._value->setValue(1.0);
			layout->addWidget(point._value, i + 1, 2);

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

	setWindowModality(Qt::WindowModal);
	setWindowTitle(tr("Voice Editor"));

	const auto layout = new QHBoxLayout{ this };

	_voicesView = new QTableView{ this };
	_voicesView->setModel(&_model);
	_voicesView->setItemDelegateForColumn(0, new VoiceDelegate{ _voicesView });
	_voicesView->setSelectionBehavior(QAbstractItemView::SelectRows);
	_voicesView->setSelectionMode(QAbstractItemView::SingleSelection);
	_voicesView->horizontalHeader()->setStretchLastSection(true);
	_voicesView->horizontalHeader()->setVisible(false);
	layout->addWidget(_voicesView);
	connect(_voicesView->selectionModel(), &QItemSelectionModel::currentChanged, [this](const QModelIndex& current, const QModelIndex& previous) {
		if (previous.isValid())
			_model.setVoice(previous, currentVoice());
		if (const auto voice = _model.voice(current))
			resetVoice(*voice);
	});

	const auto leftLayout = new QVBoxLayout{};
	layout->addLayout(leftLayout);

	const auto oscillationGroup = new QGroupBox{ tr("Oscillation"), this };
	createOscillationWidgets(oscillationGroup);
	leftLayout->addWidget(oscillationGroup);

	const auto amplitudeGroup = new QGroupBox{ tr("Amplitude"), this };
	createEnvelopeEditor(amplitudeGroup, _amplitudeEnvelope, 0.0);
	leftLayout->addWidget(amplitudeGroup);

	const auto frequencyGroup = new QGroupBox{ tr("Frequency"), this };
	createEnvelopeEditor(frequencyGroup, _frequencyEnvelope, 0.5);
	leftLayout->addWidget(frequencyGroup);

	const auto asymmetryGroup = new QGroupBox{ tr("Asymmetry"), this };
	createEnvelopeEditor(asymmetryGroup, _asymmetryEnvelope, 0.0);
	leftLayout->addWidget(asymmetryGroup);

	leftLayout->addItem(new QSpacerItem{ 0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding });

	const auto noteWidget = new QWidget{ this };
	noteWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

	const auto noteLayout = new QGridLayout{};
	layout->addLayout(noteLayout, 1);
	for (int row = 0; row < 10; ++row)
		for (int column = 0; column < 12; ++column)
		{
			const auto octave = 9 - row;
			const auto name = noteNames[column];
			const auto button = new QToolButton{ noteWidget };
			button->setText(QStringLiteral("%1%2").arg(name).arg(octave));
			button->setFixedSize({ 40, 30 });
			button->setStyleSheet(name[1] == '#' ? "background-color: black; color: white" : "background-color: white; color: black");
			button->setProperty("note", octave * 12 + column);
			connect(button, &QAbstractButton::clicked, this, &VoiceEditor::onNoteClicked);
			noteLayout->addWidget(button, row, column);
		}
}

VoiceEditor::~VoiceEditor() = default;

void VoiceEditor::onNoteClicked()
{
	const auto renderer = aulos::VoiceRenderer::create(currentVoice(), 48'000);
	assert(renderer);

	const auto button = qobject_cast<QAbstractButton*>(sender());
	assert(button);
	renderer->start(static_cast<aulos::Note>(button->property("note").toInt()), 1.f);

	_audioOutput->stop();
	_audioBuffer.close();
	_audioData.clear();
	_audioData.resize(65'536);
	for (int totalRendered = 0;;)
	{
		std::memset(_audioData.data() + totalRendered, 0, _audioData.size() - totalRendered);
		if (const auto rendered = renderer->render(_audioData.data() + totalRendered, _audioData.size() - totalRendered); rendered > 0)
		{
			totalRendered += static_cast<int>(rendered);
			_audioData.resize(totalRendered + 65'536);
		}
		else
		{
			_audioData.resize(totalRendered);
			break;
		}
	}
	_audioBuffer.open(QIODevice::ReadOnly);
	_audioOutput->start(&_audioBuffer);
}

void VoiceEditor::hideEvent(QHideEvent*)
{
	_audioOutput->stop();
	_model.setVoice(_voicesView->currentIndex(), currentVoice());
}

void VoiceEditor::resetVoice(const aulos::Voice& voice)
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

	_oscillationSpin->setValue(voice._oscillation);
	setEnvelope(_amplitudeEnvelope, voice._amplitudeEnvelope);
	setEnvelope(_frequencyEnvelope, voice._frequencyEnvelope);
	setEnvelope(_asymmetryEnvelope, voice._asymmetryEnvelope);
}

aulos::Voice VoiceEditor::currentVoice()
{
	aulos::Voice result;
	result._wave = aulos::Wave::Linear;
	result._oscillation = static_cast<float>(_oscillationSpin->value());
	if (auto i = _amplitudeEnvelope.begin(); i->_check->isChecked())
	{
		result._amplitudeEnvelope._initial = static_cast<float>(i->_value->value());
		for (++i; i != _amplitudeEnvelope.end() && i->_check->isChecked(); ++i)
			result._amplitudeEnvelope._changes.emplace_back(static_cast<float>(i->_delay->value()), static_cast<float>(i->_value->value()));
	}
	if (auto i = _frequencyEnvelope.begin(); i->_check->isChecked())
	{
		result._frequencyEnvelope._initial = static_cast<float>(i->_value->value());
		for (++i; i != _frequencyEnvelope.end() && i->_check->isChecked(); ++i)
			result._frequencyEnvelope._changes.emplace_back(static_cast<float>(i->_delay->value()), static_cast<float>(i->_value->value()));
	}
	if (auto i = _asymmetryEnvelope.begin(); i->_check->isChecked())
	{
		result._asymmetryEnvelope._initial = static_cast<float>(i->_value->value());
		for (++i; i != _asymmetryEnvelope.end() && i->_check->isChecked(); ++i)
			result._asymmetryEnvelope._changes.emplace_back(static_cast<float>(i->_delay->value()), static_cast<float>(i->_value->value()));
	}
	return result;
}
