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

class QGraphicsLineItem;

namespace aulos
{
	struct CompositionData;
	struct SequenceData;
}

class FragmentItem;
class TimelineItem;
class VoiceItem;

class CompositionScene final : public QGraphicsScene
{
	Q_OBJECT

public:
	CompositionScene();
	~CompositionScene() override;

	void insertFragment(size_t trackIndex, size_t offset, const std::shared_ptr<const aulos::SequenceData>&);
	void removeFragment(size_t trackIndex, size_t offset);
	void reset(const std::shared_ptr<const aulos::CompositionData>&);
	void setCurrentStep(double step);
	void setSpeed(unsigned speed);

signals:
	void insertFragmentRequested(size_t trackIndex, size_t offset, const std::shared_ptr<const aulos::SequenceData>&);
	void newSequenceRequested(size_t trackIndex, size_t offset);
	void removeFragmentRequested(size_t trackIndex, size_t offset);

private slots:
	void onEditRequested(size_t trackIndex, size_t offset, const std::shared_ptr<const aulos::SequenceData>&);

private:
	struct Track;
	FragmentItem* addFragmentItem(Track&, size_t offset, const std::shared_ptr<const aulos::SequenceData>&);

private:
	std::shared_ptr<const aulos::CompositionData> _composition;
	std::unique_ptr<TimelineItem> _timelineItem;
	std::vector<std::unique_ptr<Track>> _tracks;
	std::unique_ptr<VoiceItem> _addVoiceItem;
	std::unique_ptr<QGraphicsLineItem> _cursorItem;
};
