// This file is part of the Aulos toolkit.
// Copyright (C) Sergei Blagodarin.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <aulos/data.hpp>

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

	void insertSound(size_t offset, aulos::Note);
	void removeSound(size_t offset, aulos::Note);
	qreal setSequence(const aulos::SequenceData&, const QSize& viewSize);

signals:
	void insertingSound(size_t offset, aulos::Note);
	void noteActivated(aulos::Note);
	void removingSound(size_t offset, aulos::Note);

private:
	void insertNewSound(size_t offset, aulos::Note);
	void removeSoundItems();
	void setPianorollLength(size_t steps);

private:
	std::unique_ptr<PianorollItem> _pianorollItem;
	ElusiveItem* _rightBoundItem = nullptr;
	std::multimap<size_t, std::unique_ptr<SoundItem>> _soundItems;
};
