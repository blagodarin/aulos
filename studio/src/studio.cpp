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

#include "studio.hpp"

#include "voice_editor.hpp"
#include "voices_model.hpp"

#include <aulos.hpp>

#include <QCoreApplication>
#include <QFileDialog>
#include <QGraphicsView>
#include <QLabel>
#include <QMenuBar>
#include <QSettings>
#include <QSpinBox>
#include <QStatusBar>
#include <QToolBar>

namespace
{
	const auto recentFileKeyBase = QStringLiteral("RecentFile%1");

	QStringList loadRecentFileList()
	{
		QSettings settings;
		QStringList result;
		for (int index = 0;;)
		{
			const auto value = settings.value(recentFileKeyBase.arg(index));
			if (!value.isValid())
				break;
			result.prepend(value.toString());
			++index;
		}
		return result;
	}

	void saveRecentFileList(const QStringList& files)
	{
		QSettings settings;
		int index = 0;
		for (const auto& file : files)
			settings.setValue(recentFileKeyBase.arg(index++), file);
		for (;;)
		{
			const auto key = recentFileKeyBase.arg(index);
			if (!settings.contains(key))
				break;
			settings.remove(key);
			++index;
		}
	}
}

Studio::Studio()
	: _voicesModel{ std::make_unique<VoicesModel>() }
	, _voiceEditor{ std::make_unique<VoiceEditor>(*_voicesModel, this) }
{
	const auto fileMenu = menuBar()->addMenu(tr("&File"));
	_fileOpenAction = fileMenu->addAction(
		tr("&Open..."), [this] { openComposition(); }, Qt::CTRL + Qt::Key_O);
	_fileSaveAction = fileMenu->addAction(
		tr("&Save"), [this] {}, Qt::CTRL + Qt::Key_S);
	_fileSaveAsAction = fileMenu->addAction(
		tr("Save &As..."), [this] {}, Qt::CTRL + Qt::ALT + Qt::Key_S);
	_fileCloseAction = fileMenu->addAction(
		tr("&Close"), [this] { closeComposition(); }, Qt::CTRL + Qt::Key_W);
	fileMenu->addSeparator();
	_recentFilesMenu = fileMenu->addMenu(tr("&Recent Files"));
	for (const auto& recentFile : loadRecentFileList())
		setRecentFile(recentFile);
	_recentFilesMenu->addSeparator();
	_recentFilesMenu->addAction(tr("Clear"), [this] { clearRecentFiles(); });
	fileMenu->addSeparator();
	fileMenu->addAction(
		tr("E&xit"), [this] { close(); }, Qt::ALT + Qt::Key_F4);

	const auto toolsMenu = menuBar()->addMenu(tr("&Tools"));
	_toolsVoiceEditorAction = toolsMenu->addAction(
		tr("&Voice Editor"), [this] { _voiceEditor->show(); });

	_speedSpin = new QSpinBox{ this };
	_speedSpin->setRange(1, 32);
	_speedSpin->setSuffix("x");

	const auto toolBar = new QToolBar{ this };
	toolBar->setFloatable(false);
	toolBar->setMovable(false);
	toolBar->addAction(_fileOpenAction);
	toolBar->addAction(_fileSaveAction);
	toolBar->addSeparator();
	toolBar->addWidget(_speedSpin);
	toolBar->addSeparator();
	toolBar->addAction(_toolsVoiceEditorAction);
	addToolBar(toolBar);

	const auto centralWidget = new QGraphicsView{ this };
	setCentralWidget(centralWidget);

	_statusPath = new QLabel{ statusBar() };
	_statusPath->setTextFormat(Qt::RichText);
	statusBar()->addWidget(_statusPath);

	connect(_speedSpin, QOverload<int>::of(&QSpinBox::valueChanged), [this] {
		if (!_hasComposition)
			return;
		_changed = true;
		updateStatus();
	});
	connect(_voicesModel.get(), &QAbstractItemModel::dataChanged, [this] {
		if (!_hasComposition)
			return;
		_changed = true;
		updateStatus();
	});

	updateStatus();
}

Studio::~Studio() = default;

void Studio::clearRecentFiles()
{
	for (const auto action : _recentFilesActions)
	{
		_recentFilesMenu->removeAction(action);
		action->deleteLater();
	}
	_recentFilesActions.clear();
	::saveRecentFileList({});
}

void Studio::closeComposition()
{
	_hasComposition = false;
	_composition.reset();
	_compositionPath.clear();
	_compositionName.clear();
	_speedSpin->setValue(_speedSpin->minimum());
	_voicesModel->reset(nullptr);
	_changed = false;
	updateStatus();
}

void Studio::openComposition()
{
	const auto path = QFileDialog::getOpenFileName(this, tr("Open Composition"), {}, tr("Aulos Files (*.txt)"));
	if (!path.isNull())
		openComposition(path);
}

void Studio::openComposition(const QString& path)
{
	closeComposition();

	QFile file{ path };
	if (!file.open(QIODevice::ReadOnly))
		return;

	try
	{
		_composition = aulos::Composition::create(file.readAll().constData());
	}
	catch (const std::runtime_error&)
	{
		return;
	}

	_compositionPath = path;
	_compositionName = QFileInfo{ file }.fileName();
	_speedSpin->setValue(static_cast<int>(_composition->speed()));
	_voicesModel->reset(_composition.get());
	_hasComposition = true;
	setRecentFile(path);
	saveRecentFiles();
	updateStatus();
}

void Studio::setRecentFile(const QString& path)
{
	if (const auto i = std::find_if(_recentFilesActions.begin(), _recentFilesActions.end(), [this, &path](const QAction* action) { return action->text() == path; }); i != _recentFilesActions.end())
	{
		const auto action = *i;
		_recentFilesActions.erase(i);
		_recentFilesActions.prepend(action);
		_recentFilesMenu->removeAction(action);
		_recentFilesMenu->insertAction(_recentFilesMenu->actions().first(), action);
	}
	else
	{
		const auto action = new QAction{ path, this };
		connect(action, &QAction::triggered, [this, path] { openComposition(path); });
		_recentFilesActions.prepend(action);
		_recentFilesMenu->insertAction(_recentFilesMenu->actions().value(0, nullptr), action);
		while (_recentFilesActions.size() > 10)
		{
			const auto oldAction = _recentFilesActions.takeLast();
			_recentFilesMenu->removeAction(oldAction);
			delete oldAction;
		}
	}
}

void Studio::saveRecentFiles()
{
	QStringList recentFiles;
	for (const auto action : _recentFilesActions)
		recentFiles << action->text();
	::saveRecentFileList(recentFiles);
}

void Studio::updateStatus()
{
	setWindowTitle(_hasComposition ? QStringLiteral("%1 - %2").arg(_changed ? '*' + _compositionName : _compositionName, QCoreApplication::applicationName()) : QCoreApplication::applicationName());
	_fileSaveAction->setEnabled(_changed);
	_fileSaveAsAction->setEnabled(_hasComposition);
	_fileCloseAction->setEnabled(_hasComposition);
	_toolsVoiceEditorAction->setEnabled(_hasComposition);
	_speedSpin->setEnabled(_hasComposition);
	_statusPath->setText(_hasComposition ? _compositionPath : QStringLiteral("<i>%1</i>").arg(tr("no file")));
}
