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

#pragma once

#include <array>
#include <memory>
#include <vector>

#include <QGraphicsScene>

class QStaticText;

namespace aulos
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
class TimelineItem;
class VoiceItem;

class CompositionScene final : public QGraphicsScene
{
	Q_OBJECT

public:
	CompositionScene();
	~CompositionScene() override;

	void addTrack(const void* voiceId, const void* trackId);
	void appendPart(const std::shared_ptr<aulos::PartData>&);
	void insertFragment(const void* voiceId, const void* trackId, size_t offset, const std::shared_ptr<aulos::SequenceData>&);
	void removeFragment(const void* trackId, size_t offset);
	void removeTrack(const void* voiceId, const void* trackId);
	void removeVoice(const void* voiceId);
	void reset(const std::shared_ptr<aulos::CompositionData>&, size_t viewWidth);
	void selectSequence(const void* voiceId, const void* trackId, const void* sequenceId);
	QRectF setCurrentStep(double step);
	void setSpeed(unsigned speed);
	void showCursor(bool);
	size_t startOffset() const;
	void updateSequence(const void* trackId, const std::shared_ptr<aulos::SequenceData>&);
	void updateVoice(const void* id, const std::string& name);

signals:
	void newVoiceRequested();
	void fragmentMenuRequested(const void* voiceId, const void* trackId, size_t offset, const QPoint& pos);
	void sequenceSelected(const void* voiceId, const void* trackId, const void* sequenceId);
	void trackActionRequested(const void* voiceId, const void* trackId);
	void trackMenuRequested(const void* voiceId, const void* trackId, size_t offset, const QPoint& pos);
	void voiceActionRequested(const void* voiceId);
	void voiceMenuRequested(const void* voiceId, const QPoint& pos);

private slots:
	void setCompositionLength(size_t length);

private:
	struct Track;
	using TrackIterator = std::vector<std::unique_ptr<Track>>::iterator;

	FragmentItem* addFragmentItem(const void* voiceId, TrackIterator, size_t offset, const std::shared_ptr<aulos::SequenceData>&);
	TrackIterator addTrackItem(const void* voiceId, const void* trackId, size_t trackIndex, bool isFirstTrack);
	VoiceItem* addVoiceItem(const void* id, const QString& name, size_t trackCount);
	void highlightSequence(const void* trackId, const void* sequenceId);
	void highlightVoice(const void* id, bool highlight);
	std::vector<FragmentSound> makeSequenceTexts(const aulos::SequenceData&) const;
	qreal requiredVoiceColumnWidth() const;
	void setVoiceColumnWidth(qreal);
	void updateSceneRect(size_t compositionLength);

private:
	std::shared_ptr<aulos::CompositionData> _composition;
	std::vector<std::unique_ptr<VoiceItem>> _voices;
	std::unique_ptr<AddVoiceItem> _addVoiceItem;
	std::unique_ptr<CompositionItem> _compositionItem;
	TimelineItem* const _timelineItem;
	ElusiveItem* const _rightBoundItem;
	CursorItem* const _cursorItem;
	std::vector<std::unique_ptr<Track>> _tracks;
	std::array<std::shared_ptr<QStaticText>, 7 * 10> _baseNoteNames; // C0, D0, ..., C1, D1, ...
	std::array<std::shared_ptr<QStaticText>, 7> _extraNoteNames;     // C#, D#, ...
	qreal _voiceColumnWidth;
	const void* _selectedVoiceId = nullptr;
	const void* _selectedTrackId = nullptr;
	const void* _selectedSequenceId = nullptr;
};
