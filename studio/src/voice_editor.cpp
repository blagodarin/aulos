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

#include "player.hpp"
#include "voices_model.hpp"

#include <aulos/data.hpp>

#include <array>
#include <cassert>

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
#include <QMessageBox>
#include <QPushButton>
#include <QShowEvent>
#include <QStyledItemDelegate>
#include <QTableView>
#include <QToolBar>
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
	, _player{ std::make_unique<Player>() }
{
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

	const auto rootLayout = new QGridLayout{ this };

	const auto toolBar = new QToolBar{ this };
	rootLayout->addWidget(toolBar, 0, 0);

	toolBar->addAction(tr("Add"), [this] {
		aulos::Voice voice;
		voice._amplitudeEnvelope._changes = { { .1f, 1.f }, { .4f, .5f }, { .5f, 0.f } };
		voice._name = tr("NewVoice").toStdString();
		_voicesView->setCurrentIndex(_model.addVoice(voice));
	});

	const auto removeVoiceAction = toolBar->addAction(tr("Remove"), [this] {
		const auto index = _voicesView->currentIndex();
		if (QMessageBox::question(this, {}, tr("Remove \"%1\"?").arg(index.data(Qt::EditRole).toString()), QMessageBox::Yes | QMessageBox::No, QMessageBox::No) == QMessageBox::Yes)
			_model.removeVoice(index);
	});

	_voicesView = new QTableView{ this };
	_voicesView->setModel(&_model);
	_voicesView->setItemDelegateForColumn(0, new VoiceDelegate{ _voicesView });
	_voicesView->setSelectionBehavior(QAbstractItemView::SelectRows);
	_voicesView->setSelectionMode(QAbstractItemView::SingleSelection);
	_voicesView->horizontalHeader()->setStretchLastSection(true);
	_voicesView->horizontalHeader()->setVisible(false);
	rootLayout->addWidget(_voicesView, 1, 0);

	const auto currentVoiceWidget = new QWidget{ this };
	rootLayout->addWidget(currentVoiceWidget, 0, 1, 2, 1);

	const auto currentVoiceLayout = new QHBoxLayout{ currentVoiceWidget };

	const auto controlsLayout = new QVBoxLayout{};
	currentVoiceLayout->addLayout(controlsLayout);

	const auto oscillationGroup = new QGroupBox{ tr("Oscillation"), currentVoiceWidget };
	createOscillationWidgets(oscillationGroup);
	controlsLayout->addWidget(oscillationGroup);

	const auto amplitudeGroup = new QGroupBox{ tr("Amplitude"), currentVoiceWidget };
	createEnvelopeEditor(amplitudeGroup, _amplitudeEnvelope, 0.0);
	controlsLayout->addWidget(amplitudeGroup);

	const auto frequencyGroup = new QGroupBox{ tr("Frequency"), currentVoiceWidget };
	createEnvelopeEditor(frequencyGroup, _frequencyEnvelope, 0.5);
	controlsLayout->addWidget(frequencyGroup);

	const auto asymmetryGroup = new QGroupBox{ tr("Asymmetry"), currentVoiceWidget };
	createEnvelopeEditor(asymmetryGroup, _asymmetryEnvelope, 0.0);
	controlsLayout->addWidget(asymmetryGroup);

	controlsLayout->addItem(new QSpacerItem{ 0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding });

	const auto noteLayout = new QGridLayout{};
	currentVoiceLayout->addLayout(noteLayout, 1);
	for (int row = 0; row < 10; ++row)
		for (int column = 0; column < 12; ++column)
		{
			const auto octave = 9 - row;
			const auto name = noteNames[column];
			const auto button = new QToolButton{ currentVoiceWidget };
			button->setText(QStringLiteral("%1%2").arg(name).arg(octave));
			button->setFixedSize({ 40, 30 });
			button->setStyleSheet(name[1] == '#' ? "background-color: black; color: white" : "background-color: white; color: black");
			button->setProperty("note", octave * 12 + column);
			connect(button, &QAbstractButton::clicked, this, &VoiceEditor::onNoteClicked);
			noteLayout->addWidget(button, row, column);
		}

	connect(_voicesView->selectionModel(), &QItemSelectionModel::currentChanged, [this, removeVoiceAction, currentVoiceWidget](const QModelIndex& current, const QModelIndex& previous) {
		if (previous.isValid())
			_model.setVoice(previous, currentVoice());
		removeVoiceAction->setEnabled(current.isValid());
		resetVoice(_model.voice(current));
		currentVoiceWidget->setEnabled(current.isValid());
	});
}

VoiceEditor::~VoiceEditor() = default;

void VoiceEditor::onNoteClicked()
{
	const auto renderer = aulos::VoiceRenderer::create(currentVoice(), Player::SamplingRate);
	assert(renderer);

	const auto button = qobject_cast<QAbstractButton*>(sender());
	assert(button);
	renderer->start(static_cast<aulos::Note>(button->property("note").toInt()), 1.f);

	_player->reset(*renderer);
	_player->start();
}

void VoiceEditor::hideEvent(QHideEvent*)
{
	_player->stop();
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
