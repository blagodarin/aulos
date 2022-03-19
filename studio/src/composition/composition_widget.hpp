// This file is part of the Aulos toolkit.
// Copyright (C) Sergei Blagodarin.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <memory>
#include <string>

#include <QWidget>

class QGraphicsView;

namespace seir::synth
{
	struct CompositionData;
	struct SequenceData;
	struct TrackData;
	struct VoiceData;
}

class CompositionScene;
class VoiceEditor;

class CompositionWidget final : public QWidget
{
	Q_OBJECT

public:
	explicit CompositionWidget(QWidget* parent);

	float selectedTrackWeight() const;
	void setComposition(const std::shared_ptr<seir::synth::CompositionData>&);
	void setInteractive(bool);
	void setPlaybackOffset(double);
	void setSpeed(unsigned speed);
	void showCursor(bool);
	size_t startOffset() const;
	void updateSelectedSequence(const std::shared_ptr<seir::synth::SequenceData>&);

signals:
	void compositionChanged();
	void selectionChanged(const std::shared_ptr<seir::synth::VoiceData>&, const std::shared_ptr<seir::synth::TrackData>&, const std::shared_ptr<seir::synth::SequenceData>&);

private:
	bool editVoiceName(const void* id, std::string&);

private:
	std::unique_ptr<VoiceEditor> _voiceEditor;
	CompositionScene* const _scene;
	QGraphicsView* _view = nullptr;
	std::shared_ptr<seir::synth::CompositionData> _composition;
};
