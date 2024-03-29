// This file is part of the Aulos toolkit.
// Copyright (C) Sergei Blagodarin.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <seir_synth/data.hpp>

#include <QWidget>

class QGraphicsView;

namespace seir::synth
{
	struct SequenceData;
}

class SequenceScene;

class SequenceWidget final : public QWidget
{
	Q_OBJECT

public:
	explicit SequenceWidget(QWidget* parent);

	void setInteractive(bool);
	void setSequence(const std::shared_ptr<seir::synth::SequenceData>&);
	std::shared_ptr<seir::synth::SequenceData> sequence() const { return _sequenceData; }

signals:
	void noteActivated(seir::synth::Note);
	void sequenceChanged();

private:
	SequenceScene* const _scene;
	QGraphicsView* _view = nullptr;
	std::shared_ptr<seir::synth::SequenceData> _sequenceData;
};
