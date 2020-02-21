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

#include <memory>
#include <vector>

#include <QGraphicsScene>

namespace aulos
{
	struct CompositionData;
	struct PartData;
	struct SequenceData;
}

class AddVoiceItem;
class CursorItem;
class FragmentItem;
class TimelineItem;
class TrackItem;
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
	void setCurrentStep(double step);
	void setSpeed(unsigned speed);
	void showCursor(bool);
	void updateSequence(const void* trackId, const std::shared_ptr<aulos::SequenceData>&);
	void updateVoice(const void* id, const std::string& name);

signals:
	void newVoiceRequested();
	void fragmentActionRequested(const void* voiceId, const void* trackId, size_t offset);
	void fragmentMenuRequested(const void* voiceId, const void* trackId, size_t offset, const QPoint& pos);
	void trackActionRequested(const void* voiceId, const void* trackId);
	void trackMenuRequested(const void* voiceId, const void* trackId, size_t offset, const QPoint& pos);
	void voiceActionRequested(const void* voiceId);
	void voiceMenuRequested(const void* voiceId, const QPoint& pos);

private slots:
	void setCompositionLength(size_t length);

private:
	struct Track;
	FragmentItem* addFragmentItem(const void* voiceId, Track&, size_t offset, const std::shared_ptr<aulos::SequenceData>&);
	TrackItem* addTrackItem(VoiceItem*, const void* trackId);
	VoiceItem* addVoiceItem(const void* id, const QString& name, size_t trackCount);
	qreal requiredVoiceColumnWidth() const;
	void setVoiceColumnWidth(qreal);
	void updateSceneRect(size_t compositionLength);

private:
	std::shared_ptr<aulos::CompositionData> _composition;
	std::vector<std::unique_ptr<VoiceItem>> _voices;
	std::unique_ptr<TimelineItem> _timelineItem;
	std::vector<std::unique_ptr<Track>> _tracks;
	std::unique_ptr<AddVoiceItem> _addVoiceItem;
	std::unique_ptr<CursorItem> _cursorItem;
	qreal _voiceColumnWidth;
};
