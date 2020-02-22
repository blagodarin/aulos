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

#include <memory>

#include <QMainWindow>

class QGraphicsView;
class QLabel;
class QSpinBox;

namespace aulos
{
	struct CompositionData;
	struct SequenceData;
	struct TrackData;
	struct Voice;
}

class CompositionScene;
class InfoEditor;
class Player;
class SequenceEditor;
class TrackEditor;
class VoiceEditor;

class Studio : public QMainWindow
{
	Q_OBJECT

public:
	Studio();
	~Studio() override;

private:
	void clearRecentFiles();
	void closeComposition();
	void createEmptyComposition();
	bool editSequence(const void* trackId, const aulos::Voice&, const std::shared_ptr<aulos::SequenceData>&);
	bool editTrack(aulos::TrackData&);
	bool editVoice(const void* id, aulos::Voice&);
	void exportComposition();
	bool maybeSaveComposition();
	bool openComposition(const QString& path);
	bool saveComposition(const QString& path) const;
	bool saveCompositionAs();
	void saveRecentFiles() const;
	void setRecentFile(const QString& path);
	void updateStatus();

private:
	void closeEvent(QCloseEvent*) override;

private:
	std::shared_ptr<aulos::CompositionData> _composition;
	std::unique_ptr<CompositionScene> _compositionScene;
	std::unique_ptr<InfoEditor> _infoEditor;
	std::unique_ptr<VoiceEditor> _voiceEditor;
	std::unique_ptr<TrackEditor> _trackEditor;
	std::unique_ptr<SequenceEditor> _sequenceEditor;
	std::unique_ptr<Player> _player;

	QString _compositionPath;
	QString _compositionFileName;

	bool _hasComposition = false;
	bool _changed = false;

	QAction* _fileNewAction;
	QAction* _fileOpenAction;
	QAction* _fileSaveAction;
	QAction* _fileSaveAsAction;
	QAction* _fileExportAction;
	QAction* _fileCloseAction;
	QMenu* _recentFilesMenu;
	QList<QAction*> _recentFilesActions;
	QAction* _editInfoAction;
	QAction* _playAction;
	QAction* _stopAction;
	QSpinBox* _speedSpin;
	QGraphicsView* _compositionView;
	QLabel* _statusPath;
};
