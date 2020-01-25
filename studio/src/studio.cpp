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

#include "composition_scene.hpp"
#include "player.hpp"
#include "voice_editor.hpp"

#include <aulos/data.hpp>

#include <QApplication>
#include <QFileDialog>
#include <QGraphicsView>
#include <QLabel>
#include <QMenuBar>
#include <QSaveFile>
#include <QScrollBar>
#include <QSettings>
#include <QSpinBox>
#include <QStatusBar>
#include <QStyle>
#include <QTimer>
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

	template <typename T>
	void writeValue(QIODevice& device, T value)
	{
		device.write(reinterpret_cast<const char*>(&value), sizeof value);
	}
}

Studio::Studio()
	: _compositionScene{ std::make_unique<CompositionScene>() }
	, _voiceEditor{ std::make_unique<VoiceEditor>(this) }
	, _player{ std::make_unique<Player>() }
{
	resize(640, 480);

	const auto fileMenu = menuBar()->addMenu(tr("&File"));
	_fileOpenAction = fileMenu->addAction(
		qApp->style()->standardIcon(QStyle::SP_DialogOpenButton), tr("&Open..."), [this] { openComposition(); }, Qt::CTRL + Qt::Key_O);
	_fileSaveAction = fileMenu->addAction(
		qApp->style()->standardIcon(QStyle::SP_DialogSaveButton), tr("&Save"), [this] {}, Qt::CTRL + Qt::Key_S);
	_fileSaveAsAction = fileMenu->addAction(
		tr("Save &As..."), [this] {}, Qt::CTRL + Qt::ALT + Qt::Key_S);
	_fileExportAction = fileMenu->addAction(
		tr("&Export..."), [this] { exportComposition(); });
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

	const auto playbackMenu = menuBar()->addMenu(tr("&Playback"));
	_playAction = playbackMenu->addAction(
		qApp->style()->standardIcon(QStyle::SP_MediaPlay), tr("&Play"), [this] {
			const auto composition = _composition->pack();
			if (!composition)
				return;
			_player->reset(*aulos::Renderer::create(*composition, Player::SamplingRate));
			_player->start();
			updateStatus();
		});
	_stopAction = playbackMenu->addAction(
		qApp->style()->standardIcon(QStyle::SP_MediaStop), tr("&Stop"), [this] {
			_player->stop();
			updateStatus();
		});

	_speedSpin = new QSpinBox{ this };
	_speedSpin->setRange(1, 32);
	_speedSpin->setSuffix("x");

	const auto toolBar = new QToolBar{ this };
	toolBar->setFloatable(false);
	toolBar->setMovable(false);
	toolBar->addAction(_fileOpenAction);
	toolBar->addAction(_fileSaveAction);
	toolBar->addSeparator();
	toolBar->addAction(_playAction);
	toolBar->addAction(_stopAction);
	toolBar->addWidget(_speedSpin);
	addToolBar(toolBar);

	_compositionView = new QGraphicsView{ this };
	_compositionView->setScene(_compositionScene.get());
	setCentralWidget(_compositionView);

	_statusPath = new QLabel{ statusBar() };
	_statusPath->setTextFormat(Qt::RichText);
	statusBar()->addWidget(_statusPath);

	connect(_speedSpin, QOverload<int>::of(&QSpinBox::valueChanged), [this] {
		if (!_hasComposition)
			return;
		_composition->_speed = static_cast<unsigned>(_speedSpin->value());
		_compositionScene->setSpeed(_composition->_speed);
		_changed = true;
		updateStatus();
	});
	connect(_player.get(), &Player::stateChanged, [this] {
		if (!_player->isPlaying())
			_compositionScene->setCurrentStep(-1);
		updateStatus();
	});
	connect(_player.get(), &Player::timeAdvanced, [this](qint64 microseconds) {
		_compositionScene->setCurrentStep(microseconds * _composition->_speed / 1'000'000.0);
	});
	connect(_compositionScene.get(), &CompositionScene::insertFragmentRequested, [this](const std::shared_ptr<aulos::TrackData>& track, size_t offset, const std::shared_ptr<aulos::SequenceData>& sequence) {
		assert(std::find(track->_sequences.begin(), track->_sequences.end(), sequence) != track->_sequences.end());
		[[maybe_unused]] const auto inserted = track->_fragments.insert_or_assign(offset, sequence).second;
		assert(inserted);
		_compositionScene->insertFragment(track.get(), offset, sequence);
		_changed = true;
		updateStatus();
	});
	connect(_compositionScene.get(), &CompositionScene::newSequenceRequested, [this](const std::shared_ptr<aulos::TrackData>& track, size_t offset) {
		const auto sequence = std::make_shared<aulos::SequenceData>();
		track->_sequences.emplace_back(sequence);
		[[maybe_unused]] const auto inserted = track->_fragments.insert_or_assign(offset, sequence).second;
		assert(inserted);
		_compositionScene->insertFragment(track.get(), offset, sequence);
		_changed = true;
		updateStatus();
	});
	connect(_compositionScene.get(), &CompositionScene::newVoiceRequested, [this] {
		aulos::Voice voice;
		voice._amplitudeEnvelope._changes = { { .1f, 1.f }, { .4f, .5f }, { .5f, 0.f } };
		voice._name = tr("NewVoice").toStdString();
		_voiceEditor->setVoice(voice);
		if (_voiceEditor->exec() != QDialog::Accepted)
			return;
		const auto& part = _composition->_parts.emplace_back(std::make_shared<aulos::PartData>(std::make_shared<aulos::Voice>(_voiceEditor->voice())));
		part->_tracks.emplace_back(std::make_shared<aulos::TrackData>(1)); // TODO: Adjust track weight.
		_compositionScene->appendPart(part);
		_compositionView->horizontalScrollBar()->setValue(_compositionView->horizontalScrollBar()->minimum());
		_changed = true;
		updateStatus();
	});
	connect(_compositionScene.get(), &CompositionScene::removeFragmentRequested, [this](const std::shared_ptr<aulos::TrackData>& track, size_t offset) {
		[[maybe_unused]] const auto erased = track->_fragments.erase(offset);
		assert(erased);
		_compositionScene->removeFragment(track.get(), offset);
		_changed = true;
		updateStatus();
	});
	connect(_compositionScene.get(), &CompositionScene::voiceEditRequested, [this](const std::shared_ptr<aulos::Voice>& voice) {
		_voiceEditor->setVoice(*voice);
		if (_voiceEditor->exec() != QDialog::Accepted)
			return;
		*voice = _voiceEditor->voice();
		_compositionScene->updateVoice(voice.get());
		_compositionView->horizontalScrollBar()->setValue(_compositionView->horizontalScrollBar()->minimum());
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
	_compositionPath.clear();
	_compositionName.clear();
	_speedSpin->setValue(_speedSpin->minimum());
	_compositionScene->reset(nullptr);
	_player->stop();
	_changed = false;
	updateStatus();
}

void Studio::exportComposition()
{
	const auto composition = _composition->pack();
	if (!composition)
		return;

	const auto path = QFileDialog::getSaveFileName(this, tr("Export Composition"), {}, tr("WAV Files (*.wav)"));
	if (path.isNull())
		return;

	QSaveFile file{ path };
	if (!file.open(QIODevice::WriteOnly))
		return;

	QByteArray rawData;
	Player::renderData(rawData, *aulos::Renderer::create(*composition, Player::SamplingRate));

	constexpr size_t chunkHeaderSize = 8;
	constexpr size_t fmtChunkSize = 16;
	constexpr auto totalHeadersSize = chunkHeaderSize + 4 + chunkHeaderSize + fmtChunkSize + chunkHeaderSize;

	file.write("RIFF");
	::writeValue<uint32_t>(file, totalHeadersSize + rawData.size());
	assert(file.pos() == chunkHeaderSize);
	file.write("WAVE");
	file.write("fmt ");
	::writeValue<uint32_t>(file, fmtChunkSize);
	::writeValue<uint16_t>(file, 3);                                    // Data format: IEEE float PCM samples.
	::writeValue<uint16_t>(file, 1);                                    // Channels.
	::writeValue<uint32_t>(file, Player::SamplingRate);                 // Samples per second.
	::writeValue<uint32_t>(file, Player::SamplingRate * sizeof(float)); // Bytes per second.
	::writeValue<uint16_t>(file, sizeof(float));                        // Bytes per frame.
	::writeValue<uint16_t>(file, sizeof(float) * 8);                    // Bits per sample.
	file.write("data");
	::writeValue<uint32_t>(file, rawData.size());
	assert(file.pos() == totalHeadersSize);
	file.write(rawData);
	file.commit();
}

void Studio::openComposition()
{
	const auto path = QFileDialog::getOpenFileName(this, tr("Open Composition"), {}, tr("Aulos Files (*.aulos)"));
	if (!path.isNull())
		openComposition(path);
}

void Studio::openComposition(const QString& path)
{
	closeComposition();

	QFile file{ path };
	if (!file.open(QIODevice::ReadOnly))
		return;

	std::unique_ptr<aulos::Composition> composition;
	try
	{
		composition = aulos::Composition::create(file.readAll().constData());
	}
	catch (const std::runtime_error&)
	{
		return;
	}

	_composition = std::make_shared<aulos::CompositionData>(*composition);
	_compositionPath = path;
	_compositionName = QFileInfo{ file }.fileName();
	_speedSpin->setValue(static_cast<int>(_composition->_speed));
	_compositionScene->reset(_composition);
	_compositionView->horizontalScrollBar()->setValue(_compositionView->horizontalScrollBar()->minimum());
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
	_fileExportAction->setEnabled(_hasComposition);
	_fileCloseAction->setEnabled(_hasComposition);
	_playAction->setEnabled(_hasComposition && !_player->isPlaying());
	_stopAction->setEnabled(_hasComposition && _player->isPlaying());
	_speedSpin->setEnabled(_hasComposition && !_player->isPlaying());
	_compositionView->setEnabled(_hasComposition);
	_statusPath->setText(_hasComposition ? _compositionPath : QStringLiteral("<i>%1</i>").arg(tr("no file")));
}
