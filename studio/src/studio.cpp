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
#include <QStatusBar>

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
	fileMenu->addAction(
		tr("E&xit"), [this] { close(); }, Qt::ALT + Qt::Key_F4);

	const auto toolsMenu = menuBar()->addMenu(tr("&Tools"));
	_toolsVoiceEditorAction = toolsMenu->addAction(
		tr("&Voice Editor"), [this] { _voiceEditor->show(); });

	const auto centralWidget = new QGraphicsView{ this };
	setCentralWidget(centralWidget);

	_statusPath = new QLabel{ statusBar() };
	_statusPath->setTextFormat(Qt::RichText);
	statusBar()->addWidget(_statusPath);

	connect(_voicesModel.get(), &QAbstractItemModel::dataChanged, [this] {
		_changed = true;
		updateStatus();
	});

	updateStatus();
}

Studio::~Studio() = default;

void Studio::updateStatus()
{
	setWindowTitle(_composition ? QStringLiteral("%1 - %2").arg(_changed ? '*' + _compositionName : _compositionName, QCoreApplication::applicationName()) : QCoreApplication::applicationName());
	_fileSaveAction->setDisabled(!_changed);
	_fileSaveAsAction->setDisabled(!_composition);
	_fileCloseAction->setDisabled(!_composition);
	_toolsVoiceEditorAction->setDisabled(!_composition);
	_statusPath->setText(_composition ? _compositionPath : QStringLiteral("<i>%1</i>").arg(tr("no file")));
}

void Studio::openComposition()
{
	const auto filePath = QFileDialog::getOpenFileName(this, tr("Open Composition"), {}, tr("Aulos Files (*.txt)"));
	if (filePath.isNull())
		return;

	closeComposition();

	QFile file{ filePath };
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

	_compositionPath = filePath;
	_compositionName = QFileInfo{ file }.fileName();
	_voicesModel->reset(_composition.get());
	updateStatus();
}

void Studio::closeComposition()
{
	_composition.reset();
	_compositionPath.clear();
	_compositionName.clear();
	_voicesModel->reset(nullptr);
	_changed = false;
	updateStatus();
}
