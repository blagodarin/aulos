// This file is part of the Aulos toolkit.
// Copyright (C) Sergei Blagodarin.
// SPDX-License-Identifier: Apache-2.0

#include "voice_widget.hpp"

#include <seir_synth/data.hpp>
#include <seir_synth/shaper.hpp>

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

	void setShape(seir::synth::WaveShape shape, float parameter)
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
			case seir::synth::WaveShape::Linear: drawShape<seir::synth::LinearShaper>(painter, x, firstY, deltaX, deltaY, endX, lastY, _parameter); break;
			case seir::synth::WaveShape::Quadratic: drawShape<seir::synth::QuadraticShaper>(painter, x, firstY, deltaX, deltaY, endX, lastY, _parameter); break;
			case seir::synth::WaveShape::Cubic: drawShape<seir::synth::CubicShaper>(painter, x, firstY, deltaX, deltaY, endX, lastY, _parameter); break;
			case seir::synth::WaveShape::Quintic: drawShape<seir::synth::QuinticShaper>(painter, x, firstY, deltaX, deltaY, endX, lastY, _parameter); break;
			case seir::synth::WaveShape::Cosine: drawShape<seir::synth::CosineShaper>(painter, x, firstY, deltaX, deltaY, endX, lastY, _parameter); break;
			}
		}
	}

private:
	static constexpr int kSizeParameter = 25;
	static constexpr int kUnitSize = 2 * kSizeParameter + 1;
	static constexpr int kHeight = kUnitSize + 2;       // Unit height plus top and bottom borders.
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
	seir::synth::WaveShape _shape = seir::synth::WaveShape::Linear;
	float _parameter = 0.f;
};

struct VoiceWidget::EnvelopeChange
{
	QCheckBox* _check = nullptr;
	QSpinBox* _duration = nullptr;
	QDoubleSpinBox* _value = nullptr;
	QRadioButton* _sustain = nullptr;
};

VoiceWidget::VoiceWidget(QWidget* parent)
	: QScrollArea{ parent }
{
	setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);

	const auto widget = new QWidget{ this };
	const auto layout = new QVBoxLayout{ widget };

	const auto [trackGroup, trackLayout] = ::createGroup<QFormLayout>(tr("Track properties"), widget);
	_trackGroup = trackGroup;
	layout->addWidget(trackGroup);

	_trackWeightSpin = new QSpinBox{ trackGroup };
	trackLayout->addRow(tr("Weight:"), _trackWeightSpin);
	_trackWeightSpin->setRange(1, 255);
	connect(_trackWeightSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &VoiceWidget::updateTrackProperties);

	_polyphonyCombo = new QComboBox{ trackGroup };
	trackLayout->addRow(tr("Polyphony:"), _polyphonyCombo);
	_polyphonyCombo->addItem(tr("Chord"), static_cast<int>(seir::synth::Polyphony::Chord));
	_polyphonyCombo->addItem(tr("Full"), static_cast<int>(seir::synth::Polyphony::Full));
	connect(_polyphonyCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &VoiceWidget::updateTrackProperties);

	_headDelaySpin = new QDoubleSpinBox{ trackGroup };
	trackLayout->addRow(tr("Head delay:"), _headDelaySpin);
	_headDelaySpin->setDecimals(2);
	_headDelaySpin->setRange(0.0, 1'000.0);
	_headDelaySpin->setSingleStep(0.01);
	_headDelaySpin->setSuffix(tr("ms"));
	_headDelaySpin->setValue(0.0);
	connect(_headDelaySpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &VoiceWidget::updateTrackProperties);

	_sourceDistanceSpin = new QDoubleSpinBox{ trackGroup };
	trackLayout->addRow(tr("Source distance:"), _sourceDistanceSpin);
	_sourceDistanceSpin->setDecimals(2);
	_sourceDistanceSpin->setRange(0.0, 1'000.0);
	_sourceDistanceSpin->setSingleStep(0.01);
	_sourceDistanceSpin->setSuffix(tr("x"));
	_sourceDistanceSpin->setValue(0.0);
	connect(_sourceDistanceSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &VoiceWidget::updateTrackProperties);

	_sourceWidthSpin = new QSpinBox{ trackGroup };
	trackLayout->addRow(tr("Source width:"), _sourceWidthSpin);
	_sourceWidthSpin->setRange(0, 360);
	_sourceWidthSpin->setValue(0);
	connect(_sourceWidthSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &VoiceWidget::updateTrackProperties);

	_sourceOffsetSpin = new QSpinBox{ trackGroup };
	trackLayout->addRow(tr("Source offset:"), _sourceOffsetSpin);
	_sourceOffsetSpin->setRange(-90, 90);
	_sourceOffsetSpin->setValue(0);
	connect(_sourceOffsetSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &VoiceWidget::updateTrackProperties);

	const auto [waveShapeGroup, waveShapeLayout] = ::createGroup<QGridLayout>(tr("Wave shape"), widget);
	layout->addWidget(waveShapeGroup);

	_waveShapeCombo = new QComboBox{ waveShapeGroup };
	waveShapeLayout->addWidget(_waveShapeCombo, 0, 0);
	_waveShapeCombo->addItem(tr("Linear"), static_cast<int>(seir::synth::WaveShape::Linear));
	_waveShapeCombo->addItem(tr("Quadratic"), static_cast<int>(seir::synth::WaveShape::Quadratic));
	_waveShapeCombo->addItem(tr("Cubic"), static_cast<int>(seir::synth::WaveShape::Cubic));
	_waveShapeCombo->addItem(tr("Quintic"), static_cast<int>(seir::synth::WaveShape::Quintic));
	_waveShapeCombo->addItem(tr("Cosine"), static_cast<int>(seir::synth::WaveShape::Cosine));
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

	const auto createEnvelopeWidgets = [this, widget, layout](const QString& title, std::vector<EnvelopeChange>& envelope, double minimum) {
		const auto [group, groupLayout] = ::createGroup<QGridLayout>(title, widget);
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
			change._duration->setRange(0, seir::synth::EnvelopeChange::kMaxDuration.count());
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

			change._sustain = new QRadioButton{ group };
			change._sustain->setEnabled(false);
			change._sustain->setAutoExclusive(false);
			groupLayout->addWidget(change._sustain, i, 4);
			connect(change._sustain, &QRadioButton::toggled, [this, i, &envelope](bool checked) {
				if (checked)
					for (int j = 0; j < 5; ++j)
						if (j != i)
							envelope[j]._sustain->setChecked(false);
				updateVoice();
			});

			connect(change._check, &QCheckBox::toggled, change._duration, &QWidget::setEnabled);
			connect(change._check, &QCheckBox::toggled, change._value, &QWidget::setEnabled);
			if (i > 0)
			{
				const auto previousCheck = envelope[i - 1]._check;
				connect(previousCheck, &QCheckBox::toggled, change._check, &QWidget::setEnabled);
				connect(change._check, &QCheckBox::toggled, previousCheck, &QWidget::setDisabled);
				const auto previousSustain = envelope[i - 1]._sustain;
				connect(change._check, &QCheckBox::toggled, previousSustain, [previousSustain](bool checked) {
					if (!checked)
						previousSustain->setChecked(false);
					previousSustain->setEnabled(checked);
				});
			}
		}
	};

	const auto createOscillatorWidgets = [this, widget, layout](const QString& title, QSpinBox*& frequency, QDoubleSpinBox*& magnitude) {
		const auto [group, groupLayout] = ::createGroup<QFormLayout>(title, widget);
		layout->addWidget(group);

		frequency = new QSpinBox{ group };
		frequency->setRange(1, 127);
		frequency->setSingleStep(1);
		frequency->setSuffix(tr("Hz"));
		frequency->setValue(1);
		groupLayout->addRow(tr("Frequency:"), frequency);
		connect(frequency, QOverload<int>::of(&QSpinBox::valueChanged), this, &VoiceWidget::updateVoice);

		magnitude = new QDoubleSpinBox{ group };
		magnitude->setDecimals(2);
		magnitude->setRange(0.0, 1.0);
		magnitude->setSingleStep(0.01);
		magnitude->setValue(0.0);
		groupLayout->addRow(tr("Magnitude:"), magnitude);
		connect(magnitude, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &VoiceWidget::updateVoice);
	};

	createEnvelopeWidgets(tr("Amplitude changes"), _amplitudeEnvelope, 0);
	createOscillatorWidgets(tr("Tremolo"), _tremoloFrequencySpin, _tremoloMagnitudeSpin);
	createEnvelopeWidgets(tr("Pitch shift"), _frequencyEnvelope, -1);
	createOscillatorWidgets(tr("Vibrato"), _vibratoFrequencySpin, _vibratoMagnitudeSpin);
	createEnvelopeWidgets(tr("Wave asymmetry"), _asymmetryEnvelope, 0);
	createOscillatorWidgets(tr("Wave asymmetry LFO"), _asymmetryFrequencySpin, _asymmetryMagnitudeSpin);
	createEnvelopeWidgets(tr("Wave rectangularity"), _rectangularityEnvelope, 0);
	createOscillatorWidgets(tr("Wave rectangularity LFO"), _rectangularityFrequencySpin, _rectangularityMagnitudeSpin);

	layout->addItem(new QSpacerItem{ 0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding });

	setWidget(widget); // Must be called after the widget's layout is filled.
	updateWaveImage();
}

VoiceWidget::~VoiceWidget() = default;

void VoiceWidget::setParameters(const std::shared_ptr<seir::synth::VoiceData>& voice, const std::shared_ptr<seir::synth::TrackProperties>& trackProperties)
{
	const auto loadEnvelope = [](std::vector<EnvelopeChange>& dst, const seir::synth::Envelope& src) {
		for (auto i = dst.rbegin(); i != dst.rend(); ++i)
		{
			i->_check->setChecked(false);
			i->_duration->setValue(0);
			i->_value->setValue(0);
			i->_sustain->setChecked(false);
		}
		for (size_t i = 0; i < std::min(dst.size(), src._changes.size()); ++i)
		{
			dst[i]._check->setChecked(true);
			dst[i]._duration->setValue(src._changes[i]._duration.count());
			dst[i]._value->setValue(src._changes[i]._value);
			dst[i]._sustain->setChecked(i + 1 == src._sustainIndex);
		}
	};

	// Prevent handling changes.
	_voice.reset();
	_trackProperties.reset();

	_trackGroup->setEnabled(static_cast<bool>(trackProperties));

	seir::synth::TrackProperties defaultTrackProperties;
	const auto usedTrackProperties = trackProperties ? trackProperties.get() : &defaultTrackProperties;
	_trackWeightSpin->setValue(trackProperties ? trackProperties->_weight : 0);
	_polyphonyCombo->setCurrentIndex(_polyphonyCombo->findData(static_cast<int>(usedTrackProperties->_polyphony)));
	_headDelaySpin->setValue(usedTrackProperties->_headDelay);
	_sourceDistanceSpin->setValue(usedTrackProperties->_sourceDistance);
	_sourceWidthSpin->setValue(usedTrackProperties->_sourceWidth);
	_sourceOffsetSpin->setValue(usedTrackProperties->_sourceOffset);

	seir::synth::VoiceData defaultVoice;
	const auto usedVoice = voice ? voice.get() : &defaultVoice;
	_waveShapeCombo->setCurrentIndex(_waveShapeCombo->findData(static_cast<int>(usedVoice->_waveShape)));
	updateShapeParameter();
	_waveShapeParameterSpin->setValue(usedVoice->_waveShapeParameter);
	updateWaveImage();
	loadEnvelope(_amplitudeEnvelope, usedVoice->_amplitudeEnvelope);
	_tremoloFrequencySpin->setValue(static_cast<int>(usedVoice->_tremolo._frequency));
	_tremoloMagnitudeSpin->setValue(usedVoice->_tremolo._magnitude);
	loadEnvelope(_frequencyEnvelope, usedVoice->_frequencyEnvelope);
	_vibratoFrequencySpin->setValue(static_cast<int>(usedVoice->_vibrato._frequency));
	_vibratoMagnitudeSpin->setValue(usedVoice->_vibrato._magnitude);
	loadEnvelope(_asymmetryEnvelope, usedVoice->_asymmetryEnvelope);
	_asymmetryFrequencySpin->setValue(static_cast<int>(usedVoice->_asymmetryOscillation._frequency));
	_asymmetryMagnitudeSpin->setValue(usedVoice->_asymmetryOscillation._magnitude);
	loadEnvelope(_rectangularityEnvelope, usedVoice->_rectangularityEnvelope);
	_rectangularityFrequencySpin->setValue(static_cast<int>(usedVoice->_rectangularityOscillation._frequency));
	_rectangularityMagnitudeSpin->setValue(usedVoice->_rectangularityOscillation._magnitude);

	_voice = voice;
	_trackProperties = trackProperties;
}

void VoiceWidget::updateShapeParameter()
{
	if (const auto waveShape = static_cast<seir::synth::WaveShape>(_waveShapeCombo->currentData().toInt()); waveShape == seir::synth::WaveShape::Quadratic)
	{
		_waveShapeParameterSpin->setEnabled(true);
		_waveShapeParameterSpin->setRange(seir::synth::QuadraticShaper::kMinShape, seir::synth::QuadraticShaper::kMaxShape);
	}
	else if (waveShape == seir::synth::WaveShape::Cubic)
	{
		_waveShapeParameterSpin->setEnabled(true);
		_waveShapeParameterSpin->setRange(seir::synth::CubicShaper::kMinShape, seir::synth::CubicShaper::kMaxShape);
	}
	else if (waveShape == seir::synth::WaveShape::Quintic)
	{
		_waveShapeParameterSpin->setEnabled(true);
		_waveShapeParameterSpin->setRange(seir::synth::QuinticShaper::kMinShape, seir::synth::QuinticShaper::kMaxShape);
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
	_trackProperties->_polyphony = static_cast<seir::synth::Polyphony>(_polyphonyCombo->currentData().toInt());
	_trackProperties->_headDelay = static_cast<float>(_headDelaySpin->value());
	_trackProperties->_sourceDistance = static_cast<float>(_sourceDistanceSpin->value());
	_trackProperties->_sourceWidth = _sourceWidthSpin->value();
	_trackProperties->_sourceOffset = _sourceOffsetSpin->value();
	emit trackPropertiesChanged();
}

void VoiceWidget::updateVoice()
{
	const auto storeEnvelope = [](seir::synth::Envelope& dst, const std::vector<EnvelopeChange>& src) {
		dst._changes.clear();
		dst._sustainIndex = 0;
		for (auto i = src.begin(); i != src.end() && i->_check->isChecked(); ++i)
		{
			dst._changes.emplace_back(std::chrono::milliseconds{ i->_duration->value() }, static_cast<float>(i->_value->value()));
			if (i->_sustain->isChecked())
				dst._sustainIndex = i - src.begin() + 1;
		}
	};

	if (!_voice)
		return;
	_voice->_waveShape = static_cast<seir::synth::WaveShape>(_waveShapeCombo->currentData().toInt());
	_voice->_waveShapeParameter = static_cast<float>(_waveShapeParameterSpin->value());
	storeEnvelope(_voice->_amplitudeEnvelope, _amplitudeEnvelope);
	_voice->_tremolo._frequency = static_cast<float>(_tremoloFrequencySpin->value());
	_voice->_tremolo._magnitude = static_cast<float>(_tremoloMagnitudeSpin->value());
	storeEnvelope(_voice->_frequencyEnvelope, _frequencyEnvelope);
	_voice->_vibrato._frequency = static_cast<float>(_vibratoFrequencySpin->value());
	_voice->_vibrato._magnitude = static_cast<float>(_vibratoMagnitudeSpin->value());
	storeEnvelope(_voice->_asymmetryEnvelope, _asymmetryEnvelope);
	_voice->_asymmetryOscillation._frequency = static_cast<float>(_asymmetryFrequencySpin->value());
	_voice->_asymmetryOscillation._magnitude = static_cast<float>(_asymmetryMagnitudeSpin->value());
	storeEnvelope(_voice->_rectangularityEnvelope, _rectangularityEnvelope);
	_voice->_rectangularityOscillation._frequency = static_cast<float>(_rectangularityFrequencySpin->value());
	_voice->_rectangularityOscillation._magnitude = static_cast<float>(_rectangularityMagnitudeSpin->value());
	emit voiceChanged();
}

void VoiceWidget::updateWaveImage()
{
	_waveShapeWidget->setShape(static_cast<seir::synth::WaveShape>(_waveShapeCombo->currentData().toInt()), static_cast<float>(_waveShapeParameterSpin->value()));
}
