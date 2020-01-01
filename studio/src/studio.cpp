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
#include <QGridLayout>
#include <QHeaderView>
#include <QLabel>
#include <QMenuBar>
#include <QStatusBar>
#include <QTableView>

Studio::Studio()
	: _voicesModel{ std::make_unique<VoicesModel>() }
	, _voiceEditor{ std::make_unique<VoiceEditor>(this) }
{
	const auto fileMenu = menuBar()->addMenu(tr("&File"));
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

	const auto centralWidget = new QWidget{ this };
	setCentralWidget(centralWidget);

	const auto layout = new QGridLayout{ centralWidget };

	const auto voicesView = new QTableView{ centralWidget };
	voicesView->setModel(_voicesModel.get());
	voicesView->setSelectionBehavior(QAbstractItemView::SelectRows);
	voicesView->setSelectionMode(QAbstractItemView::SingleSelection);
	voicesView->horizontalHeader()->setStretchLastSection(true);
	voicesView->horizontalHeader()->setVisible(false);
	layout->addWidget(voicesView, 0, 0);
	connect(voicesView, &QAbstractItemView::doubleClicked, this, [this](const QModelIndex& index) {
		if (const auto voice = _voicesModel->voice(index))
		{
			_voiceEditor->setVoice(*voice);
			_voiceEditor->open();
		}
	});
	connect(_voiceEditor.get(), &QDialog::accepted, this, [this, voicesView] {
		_voicesModel->setVoice(voicesView->currentIndex(), _voiceEditor->voice());
		_changed = true;
		updateStatus();
	});

	_statusPath = new QLabel{ statusBar() };
	_statusPath->setTextFormat(Qt::RichText);
	statusBar()->addWidget(_statusPath);

	updateStatus();
}

Studio::~Studio() = default;

void Studio::updateStatus()
{
	setWindowTitle(_composition ? QStringLiteral("%1 - %2").arg(_changed ? '*' + _compositionName : _compositionName, QCoreApplication::applicationName()) : QCoreApplication::applicationName());
	_saveAction->setDisabled(!_changed);
	_saveAsAction->setDisabled(!_composition);
	_closeAction->setDisabled(!_composition);
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
