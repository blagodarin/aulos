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
#include "info_editor.hpp"
#include "player.hpp"
#include "sequence_scene.hpp"
#include "track_editor.hpp"
#include "utils.hpp"
#include "voice_editor.hpp"

#include <aulos/data.hpp>

#include <QApplication>
#include <QCloseEvent>
#include <QFileDialog>
#include <QGraphicsView>
#include <QLabel>
#include <QMenuBar>
#include <QMessageBox>
#include <QSaveFile>
#include <QScrollBar>
#include <QSettings>
#include <QSpinBox>
#include <QSplitter>
#include <QStatusBar>
#include <QStyle>
#include <QTimer>
#include <QToolBar>

namespace
{
	constexpr int kMaxRecentFiles = 10;
	const auto kRecentFileKeyBase = QStringLiteral("RecentFile%1");

	QStringList loadRecentFileList()
	{
		QSettings settings;
		QStringList result;
		for (int index = 0;;)
		{
			const auto value = settings.value(kRecentFileKeyBase.arg(index));
			if (!value.isValid())
				break;
			const auto path = value.toString();
			if (QFileInfo info{ path }; info.isAbsolute() && info.isFile())
				result.prepend(path);
			++index;
		}
		return result;
	}

	QFont makeBold(QFont&& font)
	{
		font.setBold(true);
		return std::move(font);
	}

	float makeTrackAmplitude(const aulos::CompositionData& composition, unsigned weight)
	{
		unsigned totalWeight = 0;
		for (const auto& part : composition._parts)
			for (const auto& track : part->_tracks)
				totalWeight += track->_weight;
		return static_cast<float>(weight) / totalWeight;
	}

	void saveRecentFileList(const QStringList& files)
	{
		QSettings settings;
		int index = 0;
		for (const auto& file : files)
			settings.setValue(kRecentFileKeyBase.arg(index++), file);
		for (;;)
		{
			const auto key = kRecentFileKeyBase.arg(index);
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
	, _sequenceScene{ std::make_unique<SequenceScene>() }
	, _infoEditor{ std::make_unique<InfoEditor>(this) }
	, _voiceEditor{ std::make_unique<VoiceEditor>(this) }
	, _trackEditor{ std::make_unique<TrackEditor>(this) }
	, _player{ std::make_unique<Player>() }
	, _sequenceVoice{ std::make_unique<aulos::Voice>() }
{
	resize(1280, 720);

	const auto fileMenu = menuBar()->addMenu(tr("&File"));
	_fileNewAction = fileMenu->addAction(
		qApp->style()->standardIcon(QStyle::SP_FileIcon), tr("&New"), [this] {
			if (!maybeSaveComposition())
				return;
			closeComposition();
			createEmptyComposition();
			_changed = true;
			updateStatus();
		},
		Qt::CTRL + Qt::Key_N);
	_fileOpenAction = fileMenu->addAction(
		qApp->style()->standardIcon(QStyle::SP_DialogOpenButton), tr("&Open..."), [this] {
			if (!maybeSaveComposition())
				return;
			const auto path = QFileDialog::getOpenFileName(this, tr("Open Composition"), !_compositionPath.isEmpty() ? QFileInfo{ _compositionPath }.dir().path() : QString{}, tr("Aulos Files (*.aulos)"));
			if (path.isNull())
				return;
			closeComposition();
			if (!openComposition(path))
				return;
			_changed = false;
			updateStatus();
		},
		Qt::CTRL + Qt::Key_O);
	_fileSaveAction = fileMenu->addAction(
		qApp->style()->standardIcon(QStyle::SP_DialogSaveButton), tr("&Save"), [this] {
			if (_compositionPath.isEmpty() ? !saveCompositionAs() : !saveComposition(_compositionPath))
				return;
			_changed = false;
			updateStatus();
		},
		Qt::CTRL + Qt::Key_S);
	_fileSaveAsAction = fileMenu->addAction(
		tr("Save &As..."), [this] {
			if (!saveCompositionAs())
				return;
			_changed = false;
			updateStatus();
		},
		Qt::CTRL + Qt::ALT + Qt::Key_S);
	_fileExportAction = fileMenu->addAction(
		tr("&Export..."), [this] { exportComposition(); });
	_fileCloseAction = fileMenu->addAction(
		tr("&Close"), [this] {
			if (!maybeSaveComposition())
				return;
			closeComposition();
			_changed = false;
			updateStatus();
		},
		Qt::CTRL + Qt::Key_W);
	fileMenu->addSeparator();
	_recentFilesMenu = fileMenu->addMenu(tr("&Recent Files"));
	for (const auto& recentFile : loadRecentFileList())
		setRecentFile(recentFile);
	_recentFilesMenu->addSeparator();
	_recentFilesMenu->addAction(tr("Clear"), [this] { clearRecentFiles(); });
	fileMenu->addSeparator();
	fileMenu->addAction(
		tr("E&xit"), [this] { close(); }, Qt::ALT + Qt::Key_F4);

	const auto editMenu = menuBar()->addMenu(tr("&Edit"));
	_editInfoAction = editMenu->addAction(tr("Composition &information..."), [this] {
		_infoEditor->setCompositionAuthor(QString::fromStdString(_composition->_author));
		_infoEditor->setCompositionTitle(QString::fromStdString(_composition->_title));
		if (_infoEditor->exec() != QDialog::Accepted)
			return;
		_composition->_author = _infoEditor->compositionAuthor().toStdString();
		_composition->_title = _infoEditor->compositionTitle().toStdString();
		_changed = true;
		updateStatus();
	});

	const auto playbackMenu = menuBar()->addMenu(tr("&Playback"));
	_playAction = playbackMenu->addAction(qApp->style()->standardIcon(QStyle::SP_MediaPlay), tr("&Play"), [this] {
		const auto composition = _composition->pack();
		if (!composition)
			return;
		assert(_mode == Mode::Editing);
		_player->reset(*aulos::Renderer::create(*composition, Player::SamplingRate));
		_mode = Mode::Playing;
		_player->start();
		updateStatus();
	});
	_stopAction = playbackMenu->addAction(qApp->style()->standardIcon(QStyle::SP_MediaStop), tr("&Stop"), [this] {
		assert(_mode == Mode::Playing);
		_player->stop();
		updateStatus();
	});

	_speedSpin = new QSpinBox{ this };
	_speedSpin->setRange(1, 32);
	_speedSpin->setSuffix("x");

	const auto toolBar = new QToolBar{ this };
	toolBar->setFloatable(false);
	toolBar->setMovable(false);
	toolBar->addAction(_fileNewAction);
	toolBar->addAction(_fileOpenAction);
	toolBar->addAction(_fileSaveAction);
	toolBar->addSeparator();
	toolBar->addAction(_playAction);
	toolBar->addAction(_stopAction);
	toolBar->addWidget(_speedSpin);
	addToolBar(toolBar);

	const auto splitter = new QSplitter{ Qt::Vertical, this };
	splitter->setChildrenCollapsible(false);
	setCentralWidget(splitter);

	_compositionView = new QGraphicsView{ _compositionScene.get(), splitter };
	_compositionView->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
	splitter->addWidget(_compositionView);

	_sequenceView = new QGraphicsView{ _sequenceScene.get(), splitter };
	_sequenceView->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
	splitter->addWidget(_sequenceView);

	splitter->setSizes({ 1, 1 });

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
		if (_mode != Mode::Playing)
			return;
		_compositionScene->showCursor(_player->isPlaying());
		if (!_player->isPlaying())
			_mode = Mode::Editing;
		updateStatus();
	});
	connect(_player.get(), &Player::timeAdvanced, [this](qint64 microseconds) {
		if (_mode != Mode::Playing)
			return;
		const auto sceneCursorRect = _compositionScene->setCurrentStep(microseconds * _composition->_speed / 1'000'000.0);
		QRect viewCursorRect{ _compositionView->mapFromScene(sceneCursorRect.topLeft()), _compositionView->mapFromScene(sceneCursorRect.bottomRight()) };
		const auto viewportRect = _compositionView->viewport()->rect();
		constexpr auto kMargin = 50;
		if (viewCursorRect.right() > viewportRect.right() - kMargin)
			viewCursorRect.moveRight(viewCursorRect.right() + viewportRect.width() - kMargin);
		else if (viewCursorRect.right() < viewportRect.left())
			viewCursorRect.moveRight(viewCursorRect.right() - viewportRect.width() / 2);
		else
			return;
		_compositionView->ensureVisible({ _compositionView->mapToScene(viewCursorRect.topLeft()), _compositionView->mapToScene(viewCursorRect.bottomRight()) }, 0);
	});
	connect(_compositionScene.get(), &CompositionScene::newVoiceRequested, [this] {
		_voiceEditor->setVoice(defaultVoiceData());
		if (_voiceEditor->exec() != QDialog::Accepted)
			return;
		const auto& part = _composition->_parts.emplace_back(std::make_shared<aulos::PartData>(std::make_shared<aulos::Voice>(_voiceEditor->voice())));
		part->_tracks.emplace_back(std::make_shared<aulos::TrackData>(1));
		_compositionScene->appendPart(part);
		_compositionView->horizontalScrollBar()->setValue(_compositionView->horizontalScrollBar()->minimum());
		_changed = true;
		updateStatus();
	});
	connect(_compositionScene.get(), &CompositionScene::fragmentMenuRequested, [this](const void* voiceId, const void* trackId, size_t offset, const QPoint& pos) {
		const auto part = std::find_if(_composition->_parts.cbegin(), _composition->_parts.cend(), [voiceId](const auto& partData) { return partData->_voice.get() == voiceId; });
		assert(part != _composition->_parts.cend());
		const auto track = std::find_if((*part)->_tracks.cbegin(), (*part)->_tracks.cend(), [trackId](const auto& trackData) { return trackData.get() == trackId; });
		assert(track != (*part)->_tracks.cend());
		const auto fragment = (*track)->_fragments.find(offset);
		assert(fragment != (*track)->_fragments.end());
		QMenu menu;
		const auto removeFragmentAction = menu.addAction(tr("Remove fragment"));
		menu.addSeparator();
		const auto editTrackAction = menu.addAction(tr("Edit track..."));
		const auto removeTrackAction = menu.addAction(tr("Remove track"));
		removeTrackAction->setEnabled((*part)->_tracks.size() > 1);
		if (const auto action = menu.exec(pos); action == removeFragmentAction)
		{
			_compositionScene->removeFragment(trackId, offset);
			(*track)->_fragments.erase(fragment);
		}
		else if (action == editTrackAction)
		{
			if (!editTrack(**track))
				return;
		}
		else if (action == removeTrackAction)
		{
			if (const auto message = tr("Remove %1 track %2?").arg("<b>" + QString::fromStdString((*part)->_voice->_name) + "</b>").arg(track - (*part)->_tracks.cbegin() + 1);
				QMessageBox::question(this, {}, message, QMessageBox::Yes | QMessageBox::No, QMessageBox::No) != QMessageBox::Yes)
				return;
			_compositionScene->removeTrack(voiceId, trackId);
			(*part)->_tracks.erase(track);
		}
		else
			return;
		_changed = true;
		updateStatus();
	});
	connect(_compositionScene.get(), &CompositionScene::sequenceSelected, this, &Studio::showSequence);
	connect(_compositionScene.get(), &CompositionScene::trackActionRequested, [this](const void* voiceId, const void* trackId) {
		const auto part = std::find_if(_composition->_parts.cbegin(), _composition->_parts.cend(), [voiceId](const auto& partData) { return partData->_voice.get() == voiceId; });
		assert(part != _composition->_parts.cend());
		const auto track = std::find_if((*part)->_tracks.cbegin(), (*part)->_tracks.cend(), [trackId](const auto& trackData) { return trackData.get() == trackId; });
		assert(track != (*part)->_tracks.cend());
		if (!editTrack(**track))
			return;
		_changed = true;
		updateStatus();
	});
	connect(_compositionScene.get(), &CompositionScene::trackMenuRequested, [this](const void* voiceId, const void* trackId, size_t offset, const QPoint& pos) {
		const auto part = std::find_if(_composition->_parts.cbegin(), _composition->_parts.cend(), [voiceId](const auto& partData) { return partData->_voice.get() == voiceId; });
		assert(part != _composition->_parts.cend());
		const auto track = std::find_if((*part)->_tracks.cbegin(), (*part)->_tracks.cend(), [trackId](const auto& trackData) { return trackData.get() == trackId; });
		assert(track != (*part)->_tracks.cend());
		QMenu menu;
		const auto editTrackAction = menu.addAction(tr("Edit track..."));
		editTrackAction->setFont(::makeBold(editTrackAction->font()));
		const auto insertSubmenu = menu.addMenu(tr("Insert sequence"));
		const auto sequenceCount = (*track)->_sequences.size();
		for (size_t sequenceIndex = 0; sequenceIndex < sequenceCount; ++sequenceIndex)
			insertSubmenu->addAction(::makeSequenceName(*(*track)->_sequences[sequenceIndex]))->setData(sequenceIndex);
		if (!(*track)->_sequences.empty())
			insertSubmenu->addSeparator();
		const auto newSequenceAction = insertSubmenu->addAction(tr("New sequence..."));
		const auto removeTrackAction = menu.addAction(tr("Remove track"));
		removeTrackAction->setEnabled((*part)->_tracks.size() > 1);
		if (const auto action = menu.exec(pos); action == editTrackAction)
		{
			if (!editTrack(**track))
				return;
		}
		else if (action == newSequenceAction)
		{
			const auto sequence = std::make_shared<aulos::SequenceData>();
			(*track)->_sequences.emplace_back(sequence);
			[[maybe_unused]] const auto inserted = (*track)->_fragments.insert_or_assign(offset, sequence).second;
			assert(inserted);
			_compositionScene->insertFragment(voiceId, trackId, offset, sequence);
			_compositionScene->selectSequence(voiceId, trackId, sequence.get());
		}
		else if (action == removeTrackAction)
		{
			if (const auto message = tr("Remove %1 track %2?").arg("<b>" + QString::fromStdString((*part)->_voice->_name) + "</b>").arg(track - (*part)->_tracks.cbegin() + 1);
				QMessageBox::question(this, {}, message, QMessageBox::Yes | QMessageBox::No, QMessageBox::No) != QMessageBox::Yes)
				return;
			_compositionScene->removeTrack(voiceId, trackId);
			(*part)->_tracks.erase(track);
		}
		else if (action)
		{
			const auto& sequence = (*track)->_sequences[action->data().toUInt()];
			[[maybe_unused]] const auto inserted = (*track)->_fragments.insert_or_assign(offset, sequence).second;
			assert(inserted);
			_compositionScene->insertFragment(voiceId, trackId, offset, sequence);
		}
		else
			return;
		_changed = true;
		updateStatus();
	});
	connect(_compositionScene.get(), &CompositionScene::voiceActionRequested, [this](const void* voiceId) {
		const auto part = std::find_if(_composition->_parts.cbegin(), _composition->_parts.cend(), [voiceId](const auto& partData) { return partData->_voice.get() == voiceId; });
		assert(part != _composition->_parts.cend());
		if (!editVoice(voiceId, *(*part)->_voice))
			return;
		_changed = true;
		updateStatus();
	});
	connect(_compositionScene.get(), &CompositionScene::voiceMenuRequested, [this](const void* voiceId, const QPoint& pos) {
		const auto part = std::find_if(_composition->_parts.cbegin(), _composition->_parts.cend(), [voiceId](const auto& partData) { return partData->_voice.get() == voiceId; });
		assert(part != _composition->_parts.cend());
		QMenu menu;
		const auto editVoiceAction = menu.addAction(tr("Edit voice..."));
		editVoiceAction->setFont(::makeBold(editVoiceAction->font()));
		const auto addTrackAction = menu.addAction(tr("Add track"));
		menu.addSeparator();
		const auto removeVoiceAction = menu.addAction(tr("Remove voice"));
		if (const auto action = menu.exec(pos); action == editVoiceAction)
		{
			if (!editVoice(voiceId, *(*part)->_voice))
				return;
		}
		else if (action == addTrackAction)
		{
			auto& track = (*part)->_tracks.emplace_back(std::make_shared<aulos::TrackData>(1));
			_compositionScene->addTrack(voiceId, track.get());
		}
		else if (action == removeVoiceAction)
		{
			if (const auto message = tr("Remove %1 voice?").arg("<b>" + QString::fromStdString((*part)->_voice->_name) + "</b>");
				QMessageBox::question(this, {}, message, QMessageBox::Yes | QMessageBox::No, QMessageBox::No) != QMessageBox::Yes)
				return;
			_compositionScene->removeVoice(voiceId);
			_composition->_parts.erase(part);
		}
		else
			return;
		_changed = true;
		updateStatus();
	});
	connect(_sequenceScene.get(), &SequenceScene::insertingSound, [this](size_t offset, aulos::Note note) {
		assert(_sequenceData);
		assert(_sequenceTrackId);
		size_t position = 0;
		const auto sound = std::find_if(_sequenceData->_sounds.begin(), _sequenceData->_sounds.end(), [offset, &position](const aulos::Sound& sound) mutable {
			position += sound._delay;
			return position >= offset;
		});
		if (sound == _sequenceData->_sounds.end())
		{
			assert(position < offset || position == offset && _sequenceData->_sounds.empty());
			_sequenceData->_sounds.emplace_back(offset - position, note);
		}
		else if (position > offset)
		{
			assert(position > offset);
			const auto nextDelay = position - offset;
			assert(sound->_delay > nextDelay || sound->_delay == nextDelay && sound == _sequenceData->_sounds.begin());
			const auto delay = sound->_delay - nextDelay;
			sound->_delay = nextDelay;
			_sequenceData->_sounds.emplace(sound, delay, note);
		}
		else
		{
			assert(position == offset);
			sound->_note = note;
		}
		_compositionScene->updateSequence(_sequenceTrackId, _sequenceData);
		_sequenceScene->insertSound(offset, note);
		_changed = true;
		updateStatus();
	});
	connect(_sequenceScene.get(), &SequenceScene::noteActivated, [this](aulos::Note note) {
		const auto renderer = aulos::VoiceRenderer::create(*_sequenceVoice, Player::SamplingRate);
		assert(renderer);
		renderer->start(note, _sequenceAmplitude);
		_player->reset(*renderer);
		_player->start();
	});
	connect(_sequenceScene.get(), &SequenceScene::removingSound, [this](size_t offset) {
		assert(_sequenceData);
		assert(_sequenceTrackId);
		const auto sound = std::find_if(_sequenceData->_sounds.begin(), _sequenceData->_sounds.end(), [offset, position = size_t{}](const aulos::Sound& sound) mutable {
			position += sound._delay;
			assert(position <= offset);
			return position == offset;
		});
		assert(sound != _sequenceData->_sounds.end());
		if (const auto nextSound = std::next(sound); nextSound != _sequenceData->_sounds.end())
			nextSound->_delay += sound->_delay;
		_sequenceData->_sounds.erase(sound);
		_compositionScene->updateSequence(_sequenceTrackId, _sequenceData);
		_sequenceScene->removeSound(offset);
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
	_compositionFileName.clear();
	_speedSpin->setValue(_speedSpin->minimum());
	_compositionScene->reset(nullptr, _compositionView->width());
	_player->stop();
	_mode = Mode::Editing;
}

void Studio::createEmptyComposition()
{
	assert(!_hasComposition);
	assert(_compositionPath.isEmpty());
	_composition = std::make_shared<aulos::CompositionData>();
	_composition->_speed = 6;
	const auto& part = _composition->_parts.emplace_back(std::make_shared<aulos::PartData>(std::make_shared<aulos::Voice>(defaultVoiceData())));
	part->_tracks.emplace_back(std::make_shared<aulos::TrackData>(1));
	_compositionFileName = tr("New composition");
	_speedSpin->setValue(static_cast<int>(_composition->_speed));
	_compositionScene->reset(_composition, _compositionView->width());
	_compositionView->horizontalScrollBar()->setValue(_compositionView->horizontalScrollBar()->minimum());
	_hasComposition = true;
}

aulos::Voice Studio::defaultVoiceData() const
{
	aulos::Voice voice;
	voice._amplitudeEnvelope._changes = { { .1f, 1.f }, { .4f, .5f }, { .5f, 0.f } };
	voice._name = tr("NewVoice").toStdString();
	return voice;
}

bool Studio::editTrack(aulos::TrackData& track)
{
	_trackEditor->setTrackWeight(track._weight);
	if (_trackEditor->exec() != QDialog::Accepted)
		return false;
	track._weight = _trackEditor->trackWeight();
	return true;
}

bool Studio::editVoice(const void* id, aulos::Voice& voice)
{
	_voiceEditor->setVoice(voice);
	if (_voiceEditor->exec() != QDialog::Accepted)
		return false;
	voice = _voiceEditor->voice();
	_compositionScene->updateVoice(id, voice._name);
	_compositionView->horizontalScrollBar()->setValue(_compositionView->horizontalScrollBar()->minimum());
	return true;
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

bool Studio::maybeSaveComposition()
{
	if (!_changed)
		return true;
	const auto action = QMessageBox::question(this, {}, tr("Save changes to the current composition?"), QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);
	return action == QMessageBox::Yes ? (_compositionPath.isEmpty() ? saveCompositionAs() : saveComposition(_compositionPath)) : action == QMessageBox::No;
}

bool Studio::openComposition(const QString& path)
{
	assert(!_hasComposition);

	QFile file{ path };
	if (!file.open(QIODevice::ReadOnly))
		return false;

	std::unique_ptr<aulos::Composition> composition;
	try
	{
		composition = aulos::Composition::create(file.readAll().constData());
	}
	catch (const std::runtime_error&)
	{
		return false;
	}

	_composition = std::make_shared<aulos::CompositionData>(*composition);
	_compositionPath = path;
	_compositionFileName = QFileInfo{ file }.fileName();
	_speedSpin->setValue(static_cast<int>(_composition->_speed));
	_compositionScene->reset(_composition, _compositionView->width());
	_compositionView->horizontalScrollBar()->setValue(_compositionView->horizontalScrollBar()->minimum());
	_hasComposition = true;
	setRecentFile(path);
	saveRecentFiles();
	return true;
}

bool Studio::saveComposition(const QString& path) const
{
	assert(_hasComposition);
	assert(!path.isEmpty());
	const auto composition = _composition->pack();
	assert(composition);
	const auto buffer = composition->save();
	QFile file{ path };
	if (!file.open(QIODevice::WriteOnly) || file.write(reinterpret_cast<const char*>(buffer.data()), static_cast<qint64>(buffer.size())) < static_cast<qint64>(buffer.size()))
	{
		QMessageBox::critical(const_cast<Studio*>(this), QString{}, file.errorString());
		return false;
	}
	return true;
}

bool Studio::saveCompositionAs()
{
	assert(_hasComposition);
	const auto path = QFileDialog::getSaveFileName(this, tr("Save Composition As"), _compositionPath.isEmpty() ? QString{} : QFileInfo{ _compositionPath }.dir().path(), tr("Aulos Files (*.aulos)"));
	if (path.isNull() || !saveComposition(path))
		return false;
	_compositionPath = path;
	_compositionFileName = QFileInfo{ _compositionPath }.fileName();
	setRecentFile(_compositionPath);
	return true;
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
		connect(action, &QAction::triggered, [this, path] {
			if (!maybeSaveComposition())
				return;
			closeComposition();
			openComposition(path);
			_changed = false;
			updateStatus();
		});
		_recentFilesActions.prepend(action);
		_recentFilesMenu->insertAction(_recentFilesMenu->actions().value(0, nullptr), action);
		while (_recentFilesActions.size() > kMaxRecentFiles)
		{
			const auto oldAction = _recentFilesActions.takeLast();
			_recentFilesMenu->removeAction(oldAction);
			delete oldAction;
		}
	}
}

void Studio::showSequence(const void* voiceId, const void* trackId, const void* sequenceId)
{
	if (!sequenceId)
	{
		*_sequenceVoice = aulos::Voice{};
		_sequenceTrackId = nullptr;
		_sequenceAmplitude = 1.f;
		_sequenceData.reset();
		_sequenceScene->setSequence({}, _sequenceView->size());
		return;
	}
	const auto part = std::find_if(_composition->_parts.cbegin(), _composition->_parts.cend(), [voiceId](const auto& partData) { return partData->_voice.get() == voiceId; });
	assert(part != _composition->_parts.cend());
	const auto track = std::find_if((*part)->_tracks.cbegin(), (*part)->_tracks.cend(), [trackId](const auto& trackData) { return trackData.get() == trackId; });
	assert(track != (*part)->_tracks.cend());
	const auto sequence = std::find_if((*track)->_sequences.cbegin(), (*track)->_sequences.cend(), [sequenceId](const auto& sequenceData) { return sequenceData.get() == sequenceId; });
	assert(sequence != (*track)->_sequences.end());
	*_sequenceVoice = *(*part)->_voice;
	_sequenceTrackId = trackId;
	_sequenceAmplitude = ::makeTrackAmplitude(*_composition, (*track)->_weight);
	_sequenceData = *sequence;
	const auto verticalPosition = _sequenceScene->setSequence(**sequence, _sequenceView->size());
	const auto horizontalScrollBar = _sequenceView->horizontalScrollBar();
	horizontalScrollBar->setValue(horizontalScrollBar->minimum());
	const auto verticalScrollBar = _sequenceView->verticalScrollBar();
	verticalScrollBar->setValue(verticalScrollBar->minimum() + std::lround((verticalScrollBar->maximum() - verticalScrollBar->minimum()) * verticalPosition));
}

void Studio::saveRecentFiles() const
{
	QStringList recentFiles;
	for (const auto action : _recentFilesActions)
		recentFiles << action->text();
	::saveRecentFileList(recentFiles);
}

void Studio::updateStatus()
{
	const auto compositionName = _hasComposition && !_composition->_title.empty() ? QString::fromStdString(_composition->_title) : _compositionFileName;
	setWindowTitle(_hasComposition ? QStringLiteral("%1 - %2").arg(_changed ? '*' + compositionName : compositionName, QCoreApplication::applicationName()) : QCoreApplication::applicationName());
	_fileSaveAction->setEnabled(_changed);
	_fileSaveAsAction->setEnabled(_hasComposition);
	_fileExportAction->setEnabled(_hasComposition);
	_fileCloseAction->setEnabled(_hasComposition);
	_editInfoAction->setEnabled(_hasComposition);
	_playAction->setEnabled(_hasComposition && _mode == Mode::Editing);
	_stopAction->setEnabled(_hasComposition && _mode == Mode::Playing);
	_speedSpin->setEnabled(_hasComposition && _mode == Mode::Editing);
	_compositionView->setEnabled(_hasComposition);
	_compositionView->setInteractive(_hasComposition && _mode == Mode::Editing);
	_sequenceScene->setSequenceEditable(false);
	_sequenceView->setEnabled(_hasComposition);
	_sequenceView->setInteractive(_hasComposition && _mode == Mode::Editing);
	_statusPath->setText(_compositionPath.isEmpty() ? QStringLiteral("<i>%1</i>").arg(tr("No file")) : _compositionPath);
}

void Studio::closeEvent(QCloseEvent* e)
{
	e->setAccepted(maybeSaveComposition());
}
