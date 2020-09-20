// This file is part of the Aulos toolkit.
// Copyright (C) Sergei Blagodarin.
// SPDX-License-Identifier: Apache-2.0

#include "voice_widget.hpp"

#include <aulos/data.hpp>
#include <aulos/src/shaper.hpp>

#include <cassert>

#include <QCheckBox>
#include <QComboBox>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QPainter>
#include <QSpinBox>

namespace
{
	template <typename Shaper>
	void drawShape(QPainter& painter, const QRect& rect, float parameter)
	{
		auto drawPoint = [&painter, &rect, lastY = 0](int x, int y) mutable {
			if (x > rect.left())
			{
				auto y1 = lastY;
				auto y2 = y;
				const auto step = y1 < y2 ? 1 : -1;
				while (std::abs(y2 - y1) > 1)
				{
					y1 += step;
					y2 -= step;
					if (y1 != y2)
					{
						painter.drawPoint(x - 1, y1);
						painter.drawPoint(x, y2);
					}
					else
						painter.drawPoint(x - (y1 & 1), y1);
				}
			}
			painter.drawPoint(x, y);
			lastY = y;
		};
		int x = rect.left();
		for (Shaper shaper{ { static_cast<float>(rect.top()), static_cast<float>(rect.height() - 1), static_cast<float>(rect.width() / 2), parameter, 0 } }; x < rect.left() + rect.width() / 2; ++x)
			drawPoint(x, std::lround(shaper.advance()));
		for (Shaper shaper{ { static_cast<float>(rect.bottom()), static_cast<float>(1 - rect.height()), static_cast<float>(rect.width() / 2), parameter, 0 } }; x < rect.right(); ++x)
			drawPoint(x, std::lround(shaper.advance()));
		drawPoint(x, rect.top());
	}
}

struct VoiceWidget::EnvelopeChange
{
	QCheckBox* _check = nullptr;
	QComboBox* _shape = nullptr;
	QSpinBox* _duration = nullptr;
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
		layout->addWidget(header, row, 0, 1, 5);
		++row;
	};

	createHeader(tr("Track properties"));

	_trackWeightSpin = new QSpinBox{ this };
	_trackWeightSpin->setRange(1, 255);
	layout->addWidget(new QLabel{ tr("Weight:"), this }, row, 1, 1, 2);
	layout->addWidget(_trackWeightSpin, row, 3, 1, 2);
	connect(_trackWeightSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &VoiceWidget::updateTrackProperties);
	++row;

	const auto trackFooter = new QFrame{ this };
	trackFooter->setFrameShadow(QFrame::Sunken);
	trackFooter->setFrameShape(QFrame::HLine);
	layout->addWidget(trackFooter, row, 0, 1, 5);
	++row;

	layout->addItem(new QSpacerItem{ 0, 0, QSizePolicy::Fixed, QSizePolicy::Fixed }, row, 0);

	_polyphonyCombo = new QComboBox{ this };
	_polyphonyCombo->addItem(tr("Chord"), static_cast<int>(aulos::Polyphony::Chord));
	_polyphonyCombo->addItem(tr("Full"), static_cast<int>(aulos::Polyphony::Full));
	layout->addWidget(new QLabel{ tr("Polyphony:"), this }, row, 1, 1, 2);
	layout->addWidget(_polyphonyCombo, row, 3, 1, 2);
	connect(_polyphonyCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &VoiceWidget::updateVoice);
	++row;

	createHeader(tr("Stereo parameters"));

	_stereoDelaySpin = new QDoubleSpinBox{ parent };
	_stereoDelaySpin->setDecimals(2);
	_stereoDelaySpin->setRange(-1'000.0, 1'000.0);
	_stereoDelaySpin->setSingleStep(0.01);
	_stereoDelaySpin->setSuffix(tr("ms"));
	_stereoDelaySpin->setValue(0.0);
	layout->addWidget(new QLabel{ tr("Delay:"), this }, row, 1, 1, 2);
	layout->addWidget(_stereoDelaySpin, row, 3, 1, 2);
	connect(_stereoDelaySpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &VoiceWidget::updateVoice);
	++row;

	_stereoRadiusSpin = new QDoubleSpinBox{ parent };
	_stereoRadiusSpin->setDecimals(2);
	_stereoRadiusSpin->setRange(-1'000.0, 1'000.0);
	_stereoRadiusSpin->setSingleStep(0.01);
	_stereoRadiusSpin->setSuffix(tr("ms"));
	_stereoRadiusSpin->setValue(0.0);
	layout->addWidget(new QLabel{ tr("Radius:"), this }, row, 1, 1, 2);
	layout->addWidget(_stereoRadiusSpin, row, 3, 1, 2);
	connect(_stereoRadiusSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &VoiceWidget::updateVoice);
	++row;

	_stereoPanSpin = new QDoubleSpinBox{ parent };
	_stereoPanSpin->setDecimals(2);
	_stereoPanSpin->setRange(-1.0, 1.0);
	_stereoPanSpin->setSingleStep(0.01);
	_stereoPanSpin->setValue(0.0);
	layout->addWidget(new QLabel{ tr("Pan:"), this }, row, 1, 1, 2);
	layout->addWidget(_stereoPanSpin, row, 3, 1, 2);
	connect(_stereoPanSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &VoiceWidget::updateVoice);
	++row;

	_stereoInversionCheck = new QCheckBox{ tr("Invert right channel"), this };
	layout->addWidget(_stereoInversionCheck, row, 1, 1, 4);
	connect(_stereoInversionCheck, &QCheckBox::toggled, this, &VoiceWidget::updateVoice);
	++row;

	const auto separator = new QFrame{ this };
	separator->setFrameShadow(QFrame::Sunken);
	separator->setFrameShape(QFrame::HLine);
	layout->addWidget(separator, row, 0, 1, 5);
	++row;

	const auto waveShapeLayout = new QGridLayout{};
	layout->addLayout(waveShapeLayout, row, 0, 1, 5);
	++row;

	_waveShapeCombo = new QComboBox{ this };
	_waveShapeCombo->addItem(tr("Linear"), static_cast<int>(aulos::WaveShape::Linear));
	_waveShapeCombo->addItem(tr("Quadratic"), static_cast<int>(aulos::WaveShape::Quadratic));
	_waveShapeCombo->addItem(tr("Cubic"), static_cast<int>(aulos::WaveShape::Cubic));
	_waveShapeCombo->addItem(tr("Quintic"), static_cast<int>(aulos::WaveShape::Quintic));
	_waveShapeCombo->addItem(tr("Cosine"), static_cast<int>(aulos::WaveShape::Cosine));
	waveShapeLayout->addWidget(new QLabel{ tr("Wave shape:"), this }, 0, 0);
	waveShapeLayout->addWidget(_waveShapeCombo, 0, 1);
	connect(_waveShapeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), [this] {
		updateShapeParameter();
		updateWaveImage();
		updateVoice();
	});

	_waveShapeParameterLabel = new QLabel{ tr("Shape parameter:"), this };
	waveShapeLayout->addWidget(_waveShapeParameterLabel, 1, 0);

	_waveShapeParameterSpin = new QDoubleSpinBox{ parent };
	_waveShapeParameterSpin->setDecimals(2);
	_waveShapeParameterSpin->setRange(0.0, 0.0);
	_waveShapeParameterSpin->setSingleStep(0.01);
	_waveShapeParameterSpin->setValue(0.0);
	waveShapeLayout->addWidget(_waveShapeParameterSpin, 1, 1);
	connect(_waveShapeParameterSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, [this] {
		updateWaveImage();
		updateVoice();
	});

	waveShapeLayout->addItem(new QSpacerItem{ 0, 0, QSizePolicy::Fixed, QSizePolicy::Fixed }, 2, 0);

	_waveShapeImage = new QLabel{ this };
	waveShapeLayout->addWidget(_waveShapeImage, 0, 2, 3, 1);

	const auto createEnvelopeWidgets = [this, layout, &row](std::vector<EnvelopeChange>& envelope, double minimum) {
		for (int i = 0; i < 5; ++i, ++row)
		{
			auto& change = envelope.emplace_back();

			change._check = new QCheckBox{ this };
			change._check->setChecked(false);
			change._check->setEnabled(i == 0);
			layout->addWidget(change._check, row, 1);
			connect(change._check, &QCheckBox::toggled, this, &VoiceWidget::updateVoice);

			change._shape = new QComboBox{ this };
			change._shape->addItem(tr("Linear"), static_cast<int>(aulos::EnvelopeShape::Linear));
			change._shape->addItem(tr("Smooth Quadratic (2 steps)"), static_cast<int>(aulos::EnvelopeShape::SmoothQuadratic2));
			change._shape->addItem(tr("Smooth Quadratic (4 steps)"), static_cast<int>(aulos::EnvelopeShape::SmoothQuadratic4));
			change._shape->addItem(tr("Sharp Quadratic (2 steps)"), static_cast<int>(aulos::EnvelopeShape::SharpQuadratic2));
			change._shape->addItem(tr("Sharp Quadratic (4 steps)"), static_cast<int>(aulos::EnvelopeShape::SharpQuadratic4));
			change._shape->setEnabled(false);
			layout->addWidget(change._shape, row, 2);
			connect(change._shape, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &VoiceWidget::updateVoice);

			change._duration = new QSpinBox{ this };
			change._duration->setEnabled(false);
			change._duration->setRange(0, aulos::EnvelopeChange::kMaxDuration.count());
			change._duration->setSingleStep(1);
			change._duration->setSuffix(tr("ms"));
			change._duration->setValue(0);
			layout->addWidget(change._duration, row, 3);
			connect(change._duration, QOverload<int>::of(&QSpinBox::valueChanged), this, &VoiceWidget::updateVoice);

			change._value = new QDoubleSpinBox{ this };
			change._value->setDecimals(2);
			change._value->setEnabled(false);
			change._value->setRange(minimum, 1.0);
			change._value->setSingleStep(0.01);
			change._value->setValue(0.0);
			layout->addWidget(change._value, row, 4);
			connect(change._value, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &VoiceWidget::updateVoice);

			connect(change._check, &QCheckBox::toggled, change._shape, &QWidget::setEnabled);
			connect(change._check, &QCheckBox::toggled, change._duration, &QWidget::setEnabled);
			connect(change._check, &QCheckBox::toggled, change._value, &QWidget::setEnabled);
			if (i > 0)
			{
				const auto previousCheck = envelope[i - 1]._check;
				connect(previousCheck, &QCheckBox::toggled, change._check, &QWidget::setEnabled);
				connect(change._check, &QCheckBox::toggled, previousCheck, &QWidget::setDisabled);
			}
		}
	};

	createHeader(tr("Amplitude changes"));
	createEnvelopeWidgets(_amplitudeEnvelope, 0);

	createHeader(tr("Frequency changes"));
	createEnvelopeWidgets(_frequencyEnvelope, -1);

	createHeader(tr("Asymmetry changes"));
	createEnvelopeWidgets(_asymmetryEnvelope, 0);

	createHeader(tr("Oscillation changes"));
	createEnvelopeWidgets(_oscillationEnvelope, 0);

	layout->addItem(new QSpacerItem{ 0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding }, row, 0, 1, 5);
	layout->setRowStretch(row, 1);

	updateWaveImage();
}

VoiceWidget::~VoiceWidget() = default;

void VoiceWidget::setParameters(const std::shared_ptr<aulos::VoiceData>& voice, const std::shared_ptr<aulos::TrackProperties>& trackProperties)
{
	const auto loadEnvelope = [](std::vector<EnvelopeChange>& dst, const aulos::Envelope& src) {
		for (auto i = dst.rbegin(); i != dst.rend(); ++i)
		{
			i->_check->setChecked(false);
			i->_shape->setCurrentIndex(i->_shape->findData(static_cast<int>(aulos::EnvelopeShape::Linear)));
			i->_duration->setValue(0);
			i->_value->setValue(0);
		}
		for (size_t i = 0; i < std::min(dst.size(), src._changes.size()); ++i)
		{
			dst[i]._check->setChecked(true);
			dst[i]._shape->setCurrentIndex(dst[i]._shape->findData(static_cast<int>(src._changes[i]._shape)));
			dst[i]._duration->setValue(src._changes[i]._duration.count());
			dst[i]._value->setValue(src._changes[i]._value);
		}
	};

	// Prevent handling changes.
	_voice.reset();
	_trackProperties.reset();

	aulos::VoiceData defaultVoice;
	const auto usedVoice = voice ? voice.get() : &defaultVoice;
	_trackWeightSpin->setEnabled(static_cast<bool>(trackProperties));
	_trackWeightSpin->setValue(trackProperties ? trackProperties->_weight : 0);
	_waveShapeCombo->setCurrentIndex(_waveShapeCombo->findData(static_cast<int>(usedVoice->_waveShape)));
	updateShapeParameter();
	_waveShapeParameterSpin->setValue(usedVoice->_waveShapeParameter);
	updateWaveImage();
	_stereoDelaySpin->setValue(usedVoice->_stereoDelay);
	_stereoRadiusSpin->setValue(usedVoice->_stereoRadius);
	_stereoPanSpin->setValue(usedVoice->_stereoPan);
	_stereoInversionCheck->setChecked(usedVoice->_stereoInversion);
	_polyphonyCombo->setCurrentIndex(_polyphonyCombo->findData(static_cast<int>(usedVoice->_polyphony)));
	loadEnvelope(_amplitudeEnvelope, usedVoice->_amplitudeEnvelope);
	loadEnvelope(_frequencyEnvelope, usedVoice->_frequencyEnvelope);
	loadEnvelope(_asymmetryEnvelope, usedVoice->_asymmetryEnvelope);
	loadEnvelope(_oscillationEnvelope, usedVoice->_oscillationEnvelope);

	_voice = voice;
	_trackProperties = trackProperties;
}

void VoiceWidget::updateShapeParameter()
{
	if (const auto waveShape = static_cast<aulos::WaveShape>(_waveShapeCombo->currentData().toInt()); waveShape == aulos::WaveShape::Quadratic)
	{
		_waveShapeParameterLabel->setEnabled(true);
		_waveShapeParameterSpin->setEnabled(true);
		_waveShapeParameterSpin->setRange(aulos::QuadraticShaper::kMinShape, aulos::QuadraticShaper::kMaxShape);
	}
	else if (waveShape == aulos::WaveShape::Cubic)
	{
		_waveShapeParameterLabel->setEnabled(true);
		_waveShapeParameterSpin->setEnabled(true);
		_waveShapeParameterSpin->setRange(aulos::CubicShaper::kMinShape, aulos::CubicShaper::kMaxShape);
	}
	else if (waveShape == aulos::WaveShape::Quintic)
	{
		_waveShapeParameterLabel->setEnabled(true);
		_waveShapeParameterSpin->setEnabled(true);
		_waveShapeParameterSpin->setRange(aulos::QuinticShaper::kMinShape, aulos::QuinticShaper::kMaxShape);
	}
	else
	{
		_waveShapeParameterLabel->setEnabled(false);
		_waveShapeParameterSpin->setEnabled(false);
		_waveShapeParameterSpin->setRange(0.0, 0.0);
	}
}

void VoiceWidget::updateTrackProperties()
{
	if (!_trackProperties)
		return;
	_trackProperties->_weight = _trackWeightSpin->value();
	emit trackPropertiesChanged();
}

void VoiceWidget::updateVoice()
{
	const auto storeEnvelope = [](aulos::Envelope& dst, const std::vector<EnvelopeChange>& src) {
		dst._changes.clear();
		for (auto i = src.begin(); i != src.end() && i->_check->isChecked(); ++i)
			dst._changes.emplace_back(std::chrono::milliseconds{ i->_duration->value() }, static_cast<float>(i->_value->value()), static_cast<aulos::EnvelopeShape>(i->_shape->currentData().toInt()));
	};

	if (!_voice)
		return;
	_voice->_waveShape = static_cast<aulos::WaveShape>(_waveShapeCombo->currentData().toInt());
	_voice->_waveShapeParameter = static_cast<float>(_waveShapeParameterSpin->value());
	storeEnvelope(_voice->_amplitudeEnvelope, _amplitudeEnvelope);
	storeEnvelope(_voice->_frequencyEnvelope, _frequencyEnvelope);
	storeEnvelope(_voice->_asymmetryEnvelope, _asymmetryEnvelope);
	storeEnvelope(_voice->_oscillationEnvelope, _oscillationEnvelope);
	_voice->_stereoDelay = static_cast<float>(_stereoDelaySpin->value());
	_voice->_stereoRadius = static_cast<float>(_stereoRadiusSpin->value());
	_voice->_stereoPan = static_cast<float>(_stereoPanSpin->value());
	_voice->_stereoInversion = _stereoInversionCheck->isChecked();
	_voice->_polyphony = static_cast<aulos::Polyphony>(_polyphonyCombo->currentData().toInt());
	emit voiceChanged();
}

void VoiceWidget::updateWaveImage()
{
	constexpr int size = 25;
	constexpr int width = 1 + (size * 4 + 1) + 1;
	constexpr int height = 1 + (size + 1 + size) + 1;
	QImage image(width, height, QImage::Format_ARGB32);
	{
		QPainter painter{ &image };
		painter.setPen(Qt::black);
		painter.setBrush(Qt::white);
		painter.drawRect(0, 0, width - 1, height - 1);
		painter.setPen(QPen{ Qt::lightGray, 0, Qt::DotLine });
		painter.drawLine(1, height / 2, width - 2, height / 2);
		painter.drawLine(width / 2, 1, width / 2, height - 2);
		painter.setPen(Qt::red);
		const auto rect = image.rect().adjusted(1, 1, -1, -1);
		const auto parameter = static_cast<float>(_waveShapeParameterSpin->value());
		switch (static_cast<aulos::WaveShape>(_waveShapeCombo->currentData().toInt()))
		{
		case aulos::WaveShape::Linear: ::drawShape<aulos::LinearShaper>(painter, rect, parameter); break;
		case aulos::WaveShape::Quadratic: ::drawShape<aulos::QuadraticShaper>(painter, rect, parameter); break;
		case aulos::WaveShape::Cubic: ::drawShape<aulos::CubicShaper>(painter, rect, parameter); break;
		case aulos::WaveShape::Quintic: ::drawShape<aulos::QuinticShaper>(painter, rect, parameter); break;
		case aulos::WaveShape::Cosine: ::drawShape<aulos::CosineShaper>(painter, rect, parameter); break;
		}
	}
	_waveShapeImage->setPixmap(QPixmap::fromImage(std::move(image)));
}
