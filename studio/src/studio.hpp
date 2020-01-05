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

class QLabel;
class QSpinBox;

namespace aulos
{
	class Composition;
}

class VoiceEditor;
class VoicesModel;

class Studio : public QMainWindow
{
	Q_OBJECT

public:
	Studio();
	~Studio() override;

private:
	void clearRecentFiles();
	void closeComposition();
	void openComposition();
	void openComposition(const QString& path);
	void saveRecentFiles();
	void setRecentFile(const QString& path);
	void updateStatus();

private:
	std::unique_ptr<aulos::Composition> _composition;
	QString _compositionPath;
	QString _compositionName;
	std::unique_ptr<VoicesModel> _voicesModel;
	bool _hasComposition = false;
	bool _changed = false;

	std::unique_ptr<VoiceEditor> _voiceEditor;

	QAction* _fileOpenAction;
	QAction* _fileSaveAction;
	QAction* _fileSaveAsAction;
	QAction* _fileCloseAction;
	QMenu* _recentFilesMenu;
	QList<QAction*> _recentFilesActions;
	QAction* _toolsVoiceEditorAction;
	QSpinBox* _speedSpin;
	QLabel* _statusPath;
};
