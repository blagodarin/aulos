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

#include <aulos/data.hpp>

#include <memory>
#include <optional>

#include <QMainWindow>

class QCheckBox;
class QComboBox;
class QLabel;
class QPushButton;
class QSpinBox;

class CompositionWidget;
class InfoEditor;
class Player;
class SequenceWidget;
class VoiceWidget;

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
	void exportComposition();
	bool maybeSaveComposition();
	bool openComposition(const QString& path);
	void playNote(aulos::Note);
	bool saveComposition(const QString& path) const;
	bool saveCompositionAs();
	void saveRecentFiles() const;
	void setRecentFile(const QString& path);
	void updateStatus();

private:
	void closeEvent(QCloseEvent*) override;

private:
	enum class Mode
	{
		Editing,
		Playing,
	};

	std::shared_ptr<aulos::CompositionData> _composition;
	std::unique_ptr<InfoEditor> _infoEditor;

	size_t _startStep = 0;
	std::unique_ptr<Player> _player;

	QString _compositionPath;
	QString _compositionFileName;

	Mode _mode = Mode::Editing;
	std::optional<aulos::Note> _autoRepeatNote;
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
	QComboBox* _channelsCombo;
	QComboBox* _samplingRateCombo;
	CompositionWidget* _compositionWidget;
	VoiceWidget* _voiceWidget;
	SequenceWidget* _sequenceWidget;
	QPushButton* _autoRepeatButton;
	QLabel* _statusPath;
};
