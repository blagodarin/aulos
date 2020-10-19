// This file is part of the Aulos toolkit.
// Copyright (C) Sergei Blagodarin.
// SPDX-License-Identifier: Apache-2.0

#include "voice_widget.hpp"

#include <aulos/data.hpp>
#include <aulos/src/shaper.hpp>

#include <cassert>
#include <optional>

#include <QCheckBox>
#include <QComboBox>
#include <QFormLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QPainter>
#include <QSpinBox>

namespace
{
	template <typename Layout>
	std::pair<QGroupBox*, Layout*> createGroup(const QString& title, QWidget* parent)
	{
		const auto group = new QGroupBox{ title, parent };
		group->setFlat(true);
		const auto layout = new Layout{ group };
		auto margins = layout->contentsMargins();
		margins.setLeft(0);
		margins.setRight(0);
		layout->setContentsMargins(margins);
		return { group, layout };
	}
}

class WaveShapeWidget : public QWidget
{
public:
	WaveShapeWidget(QWidget* parent)
		: QWidget{ parent }
	{
		setFixedHeight(kHeight);
		setMinimumWidth(kMinWidth);
	}

	void setShape(aulos::WaveShape shape, float parameter)
	{
		_shape = shape;
		_parameter = parameter;
		update();
	}

protected:
	void paintEvent(QPaintEvent*) override
	{
		const auto w = width();
		const auto h = height();
		QPainter painter{ this };
		painter.setPen(Qt::black);
		painter.setBrush(Qt::white);
		painter.drawRect(0, 0, w - 1, h - 1);
		painter.setPen(QPen{ Qt::lightGray, 0, Qt::DotLine });
		painter.drawLine(1, h / 2, w - 2, h / 2);
		constexpr auto deltaX = kUnitSize - 1;
		std::optional<int> lastY;
		for (auto x = 1, deltaY = kUnitSize - 1; x < w - 1; x += deltaX, deltaY = -deltaY)
		{
			if (lastY)
			{
				painter.setPen(QPen{ Qt::lightGray, 0, Qt::DotLine });
				painter.drawLine(x, 1, x, h - 2);
			}
			const auto firstY = deltaY > 0 ? 1 : h - 2;
			const auto endX = std::min(x + deltaX, w - 1);
			painter.setPen(Qt::red);
			switch (_shape)
			{
			case aulos::WaveShape::Linear: drawShape<aulos::LinearShaper>(painter, x, firstY, deltaX, deltaY, endX, lastY, _parameter); break;
			case aulos::WaveShape::Quadratic: drawShape<aulos::QuadraticShaper>(painter, x, firstY, deltaX, deltaY, endX, lastY, _parameter); break;
			case aulos::WaveShape::Cubic: drawShape<aulos::CubicShaper>(painter, x, firstY, deltaX, deltaY, endX, lastY, _parameter); break;
			case aulos::WaveShape::Quintic: drawShape<aulos::QuinticShaper>(painter, x, firstY, deltaX, deltaY, endX, lastY, _parameter); break;
			case aulos::WaveShape::Cosine: drawShape<aulos::CosineShaper>(painter, x, firstY, deltaX, deltaY, endX, lastY, _parameter); break;
			}
		}
	}

private:
	static constexpr int kSizeParameter = 25;
	static constexpr int kUnitSize = 2 * kSizeParameter + 1;
	static constexpr int kHeight = kUnitSize + 2; // Unit height plus top and bottom borders.
	static constexpr int kMinWidth = 2 * kUnitSize + 3; // Full period with two starting points plus left and right borders.

	template <typename Shaper>
	static void drawShape(QPainter& painter, int firstX, int firstY, int deltaX, int deltaY, int endX, std::optional<int>& lastY, float parameter)
	{
		auto drawPoint = [&painter, &lastY](int x, int y) mutable {
			if (lastY)
			{
				auto y1 = *lastY;
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
			lastY.emplace(y);
		};
		Shaper shaper{ { static_cast<float>(firstY), static_cast<float>(deltaY), static_cast<float>(deltaX), parameter, 0 } };
		for (int x = firstX; x < endX; ++x)
			drawPoint(x, std::lround(shaper.advance()));
	}

private:
	aulos::WaveShape _shape = aulos::WaveShape::Linear;
	float _parameter = 0.f;
};

struct VoiceWidget::EnvelopeChange
{
	QCheckBox* _check = nullptr;
	QSpinBox* _duration = nullptr;
	QDoubleSpinBox* _value = nullptr;
};

VoiceWidget::VoiceWidget(QWidget* parent)
	: QWidget{ parent }
{
	const auto layout = new QVBoxLayout{ this };

	const auto [trackGroup, trackLayout] = ::createGroup<QFormLayout>(tr("Track properties"), this);
	_trackGroup = trackGroup;
	layout->addWidget(trackGroup);

	_trackWeightSpin = new QSpinBox{ trackGroup };
	trackLayout->addRow(tr("Weight:"), _trackWeightSpin);
	_trackWeightSpin->setRange(1, 255);
	connect(_trackWeightSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &VoiceWidget::updateTrackProperties);

	_polyphonyCombo = new QComboBox{ trackGroup };
	trackLayout->addRow(tr("Polyphony:"), _polyphonyCombo);
	_polyphonyCombo->addItem(tr("Chord"), static_cast<int>(aulos::Polyphony::Chord));
	_polyphonyCombo->addItem(tr("Full"), static_cast<int>(aulos::Polyphony::Full));
	connect(_polyphonyCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &VoiceWidget::updateTrackProperties);

	_stereoDelaySpin = new QDoubleSpinBox{ trackGroup };
	trackLayout->addRow(tr("Stereo delay:"), _stereoDelaySpin);
	_stereoDelaySpin->setDecimals(2);
	_stereoDelaySpin->setRange(-1'000.0, 1'000.0);
	_stereoDelaySpin->setSingleStep(0.01);
	_stereoDelaySpin->setSuffix(tr("ms"));
	_stereoDelaySpin->setValue(0.0);
	connect(_stereoDelaySpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &VoiceWidget::updateTrackProperties);

	_stereoRadiusSpin = new QDoubleSpinBox{ trackGroup };
	trackLayout->addRow(tr("Stereo radius:"), _stereoRadiusSpin);
	_stereoRadiusSpin->setDecimals(2);
	_stereoRadiusSpin->setRange(-1'000.0, 1'000.0);
	_stereoRadiusSpin->setSingleStep(0.01);
	_stereoRadiusSpin->setSuffix(tr("ms"));
	_stereoRadiusSpin->setValue(0.0);
	connect(_stereoRadiusSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &VoiceWidget::updateTrackProperties);

	_stereoPanSpin = new QDoubleSpinBox{ trackGroup };
	trackLayout->addRow(tr("Stereo pan:"), _stereoPanSpin);
	_stereoPanSpin->setDecimals(2);
	_stereoPanSpin->setRange(-1.0, 1.0);
	_stereoPanSpin->setSingleStep(0.01);
	_stereoPanSpin->setValue(0.0);
	connect(_stereoPanSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &VoiceWidget::updateTrackProperties);

	_stereoInversionCheck = new QCheckBox{ tr("Invert right channel"), trackGroup };
	trackLayout->addRow(_stereoInversionCheck);
	connect(_stereoInversionCheck, &QCheckBox::toggled, this, &VoiceWidget::updateTrackProperties);

	const auto [waveShapeGroup, waveShapeLayout] = ::createGroup<QGridLayout>(tr("Wave shape"), this);
	layout->addWidget(waveShapeGroup);

	_waveShapeCombo = new QComboBox{ waveShapeGroup };
	waveShapeLayout->addWidget(_waveShapeCombo, 0, 0);
	_waveShapeCombo->addItem(tr("Linear"), static_cast<int>(aulos::WaveShape::Linear));
	_waveShapeCombo->addItem(tr("Quadratic"), static_cast<int>(aulos::WaveShape::Quadratic));
	_waveShapeCombo->addItem(tr("Cubic"), static_cast<int>(aulos::WaveShape::Cubic));
	_waveShapeCombo->addItem(tr("Quintic"), static_cast<int>(aulos::WaveShape::Quintic));
	_waveShapeCombo->addItem(tr("Cosine"), static_cast<int>(aulos::WaveShape::Cosine));
	connect(_waveShapeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), [this] {
		updateShapeParameter();
		updateWaveImage();
		updateVoice();
	});

	_waveShapeParameterSpin = new QDoubleSpinBox{ waveShapeGroup };
	waveShapeLayout->addWidget(_waveShapeParameterSpin, 0, 1);
	_waveShapeParameterSpin->setDecimals(2);
	_waveShapeParameterSpin->setRange(0.0, 0.0);
	_waveShapeParameterSpin->setSingleStep(0.01);
	_waveShapeParameterSpin->setValue(0.0);
	connect(_waveShapeParameterSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, [this] {
		updateWaveImage();
		updateVoice();
	});

	_waveShapeWidget = new WaveShapeWidget{ waveShapeGroup };
	waveShapeLayout->addWidget(_waveShapeWidget, 1, 0, 1, 2);

	const auto createEnvelopeWidgets = [this, layout](const QString& title, std::vector<EnvelopeChange>& envelope, double minimum) {
		const auto [group, groupLayout] = ::createGroup<QGridLayout>(title, this);
		layout->addWidget(group);
		for (int i = 0; i < 5; ++i)
		{
			auto& change = envelope.emplace_back();

			change._check = new QCheckBox{ group };
			change._check->setChecked(false);
			change._check->setEnabled(i == 0);
			groupLayout->addWidget(change._check, i, 1);
			connect(change._check, &QCheckBox::toggled, this, &VoiceWidget::updateVoice);

			change._duration = new QSpinBox{ group };
			change._duration->setEnabled(false);
			change._duration->setRange(0, aulos::EnvelopeChange::kMaxDuration.count());
			change._duration->setSingleStep(1);
			change._duration->setSuffix(tr("ms"));
			change._duration->setValue(0);
			groupLayout->addWidget(change._duration, i, 2);
			connect(change._duration, QOverload<int>::of(&QSpinBox::valueChanged), this, &VoiceWidget::updateVoice);

			change._value = new QDoubleSpinBox{ group };
			change._value->setDecimals(2);
			change._value->setEnabled(false);
			change._value->setRange(minimum, 1.0);
			change._value->setSingleStep(0.01);
			change._value->setValue(0.0);
			groupLayout->addWidget(change._value, i, 3);
			connect(change._value, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &VoiceWidget::updateVoice);

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

	createEnvelopeWidgets(tr("Amplitude changes"), _amplitudeEnvelope, 0);
	createEnvelopeWidgets(tr("Frequency changes") , _frequencyEnvelope, -1);
	createEnvelopeWidgets(tr("Asymmetry changes"), _asymmetryEnvelope, 0);
	createEnvelopeWidgets(tr("Oscillation changes"), _oscillationEnvelope, 0);

	layout->addItem(new QSpacerItem{ 0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding });

	updateWaveImage();
}

VoiceWidget::~VoiceWidget() = default;

void VoiceWidget::setParameters(const std::shared_ptr<aulos::VoiceData>& voice, const std::shared_ptr<aulos::TrackProperties>& trackProperties)
{
	const auto loadEnvelope = [](std::vector<EnvelopeChange>& dst, const aulos::Envelope& src) {
		for (auto i = dst.rbegin(); i != dst.rend(); ++i)
		{
			i->_check->setChecked(false);
			i->_duration->setValue(0);
			i->_value->setValue(0);
		}
		for (size_t i = 0; i < std::min(dst.size(), src._changes.size()); ++i)
		{
			dst[i]._check->setChecked(true);
			dst[i]._duration->setValue(src._changes[i]._duration.count());
			dst[i]._value->setValue(src._changes[i]._value);
		}
	};

	// Prevent handling changes.
	_voice.reset();
	_trackProperties.reset();

	_trackGroup->setEnabled(static_cast<bool>(trackProperties));

	aulos::TrackProperties defaultTrackProperties;
	const auto usedTrackProperties = trackProperties ? trackProperties.get() : &defaultTrackProperties;
	_trackWeightSpin->setValue(trackProperties ? trackProperties->_weight : 0);
	_polyphonyCombo->setCurrentIndex(_polyphonyCombo->findData(static_cast<int>(usedTrackProperties->_polyphony)));
	_stereoDelaySpin->setValue(usedTrackProperties->_stereoDelay);
	_stereoRadiusSpin->setValue(usedTrackProperties->_stereoRadius);
	_stereoPanSpin->setValue(usedTrackProperties->_stereoPan);
	_stereoInversionCheck->setChecked(usedTrackProperties->_stereoInversion);

	aulos::VoiceData defaultVoice;
	const auto usedVoice = voice ? voice.get() : &defaultVoice;
	_waveShapeCombo->setCurrentIndex(_waveShapeCombo->findData(static_cast<int>(usedVoice->_waveShape)));
	updateShapeParameter();
	_waveShapeParameterSpin->setValue(usedVoice->_waveShapeParameter);
	updateWaveImage();
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
		_waveShapeParameterSpin->setEnabled(true);
		_waveShapeParameterSpin->setRange(aulos::QuadraticShaper::kMinShape, aulos::QuadraticShaper::kMaxShape);
	}
	else if (waveShape == aulos::WaveShape::Cubic)
	{
		_waveShapeParameterSpin->setEnabled(true);
		_waveShapeParameterSpin->setRange(aulos::CubicShaper::kMinShape, aulos::CubicShaper::kMaxShape);
	}
	else if (waveShape == aulos::WaveShape::Quintic)
	{
		_waveShapeParameterSpin->setEnabled(true);
		_waveShapeParameterSpin->setRange(aulos::QuinticShaper::kMinShape, aulos::QuinticShaper::kMaxShape);
	}
	else
	{
		_waveShapeParameterSpin->setEnabled(false);
		_waveShapeParameterSpin->setRange(0.0, 0.0);
	}
}

void VoiceWidget::updateTrackProperties()
{
	if (!_trackProperties)
		return;
	_trackProperties->_weight = _trackWeightSpin->value();
	_trackProperties->_polyphony = static_cast<aulos::Polyphony>(_polyphonyCombo->currentData().toInt());
	_trackProperties->_stereoDelay = static_cast<float>(_stereoDelaySpin->value());
	_trackProperties->_stereoRadius = static_cast<float>(_stereoRadiusSpin->value());
	_trackProperties->_stereoPan = static_cast<float>(_stereoPanSpin->value());
	_trackProperties->_stereoInversion = _stereoInversionCheck->isChecked();
	emit trackPropertiesChanged();
}

void VoiceWidget::updateVoice()
{
	const auto storeEnvelope = [](aulos::Envelope& dst, const std::vector<EnvelopeChange>& src) {
		dst._changes.clear();
		for (auto i = src.begin(); i != src.end() && i->_check->isChecked(); ++i)
			dst._changes.emplace_back(std::chrono::milliseconds{ i->_duration->value() }, static_cast<float>(i->_value->value()));
	};

	if (!_voice)
		return;
	_voice->_waveShape = static_cast<aulos::WaveShape>(_waveShapeCombo->currentData().toInt());
	_voice->_waveShapeParameter = static_cast<float>(_waveShapeParameterSpin->value());
	storeEnvelope(_voice->_amplitudeEnvelope, _amplitudeEnvelope);
	storeEnvelope(_voice->_frequencyEnvelope, _frequencyEnvelope);
	storeEnvelope(_voice->_asymmetryEnvelope, _asymmetryEnvelope);
	storeEnvelope(_voice->_oscillationEnvelope, _oscillationEnvelope);
	emit voiceChanged();
}

void VoiceWidget::updateWaveImage()
{
	_waveShapeWidget->setShape(static_cast<aulos::WaveShape>(_waveShapeCombo->currentData().toInt()), static_cast<float>(_waveShapeParameterSpin->value()));
}
