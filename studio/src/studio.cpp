//
// This file is part of the Aulos toolkit.
// Copyright (C) 2019 Sergei Blagodarin.
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

#include <aulos.hpp>

#include <QCoreApplication>
#include <QFileDialog>
#include <QMenuBar>

Studio::Studio()
	: _voiceEditor{ std::make_unique<VoiceEditor>(this) }
{
	setMinimumSize({ 400, 300 });

	const auto menuBar = new QMenuBar{ this };

	const auto fileMenu = menuBar->addMenu(tr("&File"));
	_openAction = fileMenu->addAction(
		tr("&Open..."), [this] { openComposition(); }, Qt::CTRL + Qt::Key_O);
	_saveAction = fileMenu->addAction(
		tr("&Save"), [this] {}, Qt::CTRL + Qt::Key_S);
	_saveAsAction = fileMenu->addAction(
		tr("Save &As..."), [this] {}, Qt::CTRL + Qt::ALT + Qt::Key_S);
	_closeAction = fileMenu->addAction(
		tr("&Close"), [this] { closeComposition(); }, Qt::CTRL + Qt::Key_W);
	fileMenu->addSeparator();
	fileMenu->addAction(
		tr("E&xit"), [this] { close(); }, Qt::ALT + Qt::Key_F4);

	const auto toolsMenu = menuBar->addMenu(tr("&Tools"));
	toolsMenu->addAction(tr("&Voice Editor"), [this] { _voiceEditor->open(); });

	updateStatus();
}

Studio::~Studio() = default;

void Studio::updateStatus()
{
	setWindowTitle(_compositionName.isEmpty() ? QCoreApplication::applicationName() : QStringLiteral("%1 - %2").arg(_compositionName, QCoreApplication::applicationName()));
	_saveAction->setDisabled(true);
	_saveAsAction->setDisabled(true);
	_closeAction->setDisabled(!_composition);
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

	_compositionName = filePath;
	updateStatus();
}

void Studio::closeComposition()
{
	_compositionName.clear();
	_composition.reset();
	updateStatus();
}
