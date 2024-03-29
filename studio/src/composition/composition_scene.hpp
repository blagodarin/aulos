// This file is part of the Aulos toolkit.
// Copyright (C) Sergei Blagodarin.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <array>
#include <memory>
#include <vector>

#include <QGraphicsScene>

class QStaticText;

namespace seir::synth
{
	struct CompositionData;
	struct PartData;
	struct SequenceData;
}

class AddVoiceItem;
class CompositionItem;
class CursorItem;
class ElusiveItem;
class FragmentItem;
struct FragmentSound;
class LoopItem;
class TimelineItem;
class VoiceItem;

class CompositionScene final : public QGraphicsScene
{
	Q_OBJECT

public:
	explicit CompositionScene(QObject* parent = nullptr);
	~CompositionScene() override;

	void addTrack(const void* voiceId, const void* trackId);
	void appendPart(const std::shared_ptr<seir::synth::PartData>&);
	void insertFragment(const void* voiceId, const void* trackId, size_t offset, const std::shared_ptr<seir::synth::SequenceData>&);
	void removeFragment(const void* trackId, size_t offset);
	void removeTrack(const void* voiceId, const void* trackId);
	void removeVoice(const void* voiceId);
	void reset(const std::shared_ptr<seir::synth::CompositionData>&, size_t viewWidth);
	void selectFragment(const void* voiceId, const void* trackId, const void* sequenceId, size_t offset);
	float selectedTrackWeight() const;
	QRectF setCurrentStep(double step);
	void setSpeed(unsigned speed);
	void showCursor(bool);
	size_t startOffset() const;
	void updateLoop();
	void updateSelectedSequence(const std::shared_ptr<seir::synth::SequenceData>&);
	void updateVoice(const void* id, const std::string& name);

signals:
	void loopMenuRequested(const QPoint& pos);
	void newVoiceRequested();
	void fragmentMenuRequested(const void* voiceId, const void* trackId, size_t offset, const QPoint& pos);
	void fragmentSelected(const void* voiceId, const void* trackId, const void* sequenceId, size_t offset);
	void timelineMenuRequested(size_t step, const QPoint& pos);
	void trackMenuRequested(const void* voiceId, const void* trackId, size_t offset, const QPoint& pos);
	void voiceActionRequested(const void* voiceId);
	void voiceMenuRequested(const void* voiceId, const QPoint& pos);

private slots:
	void setCompositionLength(size_t length);

private:
	struct Track;
	using TrackIterator = std::vector<std::unique_ptr<Track>>::iterator;

	FragmentItem* addFragmentItem(const void* voiceId, TrackIterator, size_t offset, const std::shared_ptr<seir::synth::SequenceData>&);
	TrackIterator addTrackItem(const void* voiceId, const void* trackId, size_t trackIndex, bool isFirstTrack);
	VoiceItem* addVoiceItem(const void* id, const QString& name, size_t trackCount);
	void highlightSequence(const void* trackId, const void* sequenceId, size_t offset);
	void highlightVoice(const void* id, bool highlight);
	std::vector<FragmentSound> makeSequenceTexts(const seir::synth::SequenceData&) const;
	qreal requiredVoiceColumnWidth() const;
	void setVoiceColumnWidth(qreal);
	void updateSceneRect(size_t compositionLength);

private:
	std::shared_ptr<seir::synth::CompositionData> _composition;
	std::vector<std::unique_ptr<VoiceItem>> _voices;
	std::unique_ptr<AddVoiceItem> _addVoiceItem;
	std::unique_ptr<CompositionItem> _compositionItem;
	TimelineItem* const _timelineItem;
	ElusiveItem* const _rightBoundItem;
	CursorItem* const _cursorItem;
	LoopItem* const _loopItem;
	std::vector<std::unique_ptr<Track>> _tracks;
	std::array<std::shared_ptr<QStaticText>, 7 * 10> _baseNoteNames; // C0, D0, ..., C1, D1, ...
	std::array<std::shared_ptr<QStaticText>, 7> _extraNoteNames;     // C#, D#, ...
	qreal _voiceColumnWidth;
	const void* _selectedVoiceId = nullptr;
	const void* _selectedTrackId = nullptr;
	const void* _selectedSequenceId = nullptr;
	size_t _selectedFragmentOffset = 0;
};
