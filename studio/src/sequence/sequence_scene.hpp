// This file is part of the Aulos toolkit.
// Copyright (C) Sergei Blagodarin.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <seir_synth/data.hpp>

#include <memory>

#include <QGraphicsScene>

class ElusiveItem;
class PianorollItem;
class SoundItem;

class SequenceScene final : public QGraphicsScene
{
	Q_OBJECT

public:
	explicit SequenceScene(QObject* parent = nullptr);
	~SequenceScene() override;

	void insertSound(size_t offset, seir::synth::Note);
	void removeSound(size_t offset, seir::synth::Note);
	qreal setSequence(const seir::synth::SequenceData&, const QSize& viewSize);
	void setSoundSustain(size_t offset, seir::synth::Note, size_t sustain);

signals:
	void decreasingSustain(size_t offset, seir::synth::Note);
	void increasingSustain(size_t offset, seir::synth::Note);
	void insertingSound(size_t offset, seir::synth::Note);
	void noteActivated(seir::synth::Note);
	void removingSound(size_t offset, seir::synth::Note);

private:
	void insertNewSound(size_t offset, seir::synth::Note, size_t sustain);
	void removeSoundItems();
	void setPianorollLength(size_t steps);

private:
	std::unique_ptr<PianorollItem> _pianorollItem;
	ElusiveItem* _rightBoundItem = nullptr;
	std::multimap<size_t, std::unique_ptr<SoundItem>> _soundItems;
};
