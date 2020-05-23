// This file is part of the Aulos toolkit.
// Copyright (C) Sergei Blagodarin.
// SPDX-License-Identifier: Apache-2.0

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
	QCheckBox* _loopPlaybackCheck;
	CompositionWidget* _compositionWidget;
	VoiceWidget* _voiceWidget;
	SequenceWidget* _sequenceWidget;
	QPushButton* _autoRepeatButton;
	QLabel* _statusPath;
};
