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

#include "composition/composition_widget.hpp"
#include "sequence/sequence_widget.hpp"
#include "info_editor.hpp"
#include "player.hpp"
#include "theme.hpp"
#include "voice_widget.hpp"

#include <aulos/data.hpp>

#include <cassert>

#include <QApplication>
#include <QCloseEvent>
#include <QComboBox>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QLabel>
#include <QMenuBar>
#include <QMessageBox>
#include <QPushButton>
#include <QSaveFile>
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

	std::shared_ptr<aulos::VoiceData> makeDefaultVoice()
	{
		using namespace std::chrono_literals;
		auto voice = std::make_shared<aulos::VoiceData>();
		voice->_amplitudeEnvelope._changes = {
			{ 100ms, 1.f, aulos::EnvelopeShape::Linear },
			{ 400ms, .5f, aulos::EnvelopeShape::Linear },
			{ 500ms, 0.f, aulos::EnvelopeShape::Linear },
		};
		return voice;
	}

	QSizePolicy makeExpandingSizePolicy(int horizontalStretch, int verticalStretch)
	{
		QSizePolicy policy{ QSizePolicy::Expanding, QSizePolicy::Expanding };
		policy.setHorizontalStretch(horizontalStretch);
		policy.setVerticalStretch(verticalStretch);
		return policy;
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
	: _infoEditor{ std::make_unique<InfoEditor>(this) }
	, _player{ std::make_unique<Player>() }
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
		_autoRepeatButton->setChecked(false);
		const auto renderer = aulos::Renderer::create(*composition, _samplingRateCombo->currentData().toUInt(), _channelsCombo->currentData().toUInt());
		[[maybe_unused]] const auto skippedBytes = renderer->render(nullptr, _compositionWidget->startOffset() * renderer->samplingRate() * renderer->channels() * sizeof(float) / _composition->_speed);
		_player->reset(*renderer);
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

	_channelsCombo = new QComboBox{ this };
	_channelsCombo->addItem(tr("Stereo"), 2u);
	_channelsCombo->addItem(tr("Mono"), 1u);

	_samplingRateCombo = new QComboBox{ this };
	const auto hz = tr("%L1 Hz");
	for (const auto samplingRate : { 48'000u, 44'100u, 32'000u, 24'000u, 22'050u, 16'000u, 11'025u, 8'000u })
		_samplingRateCombo->addItem(hz.arg(samplingRate), samplingRate);

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
	toolBar->addSeparator();
	toolBar->addWidget(_channelsCombo);
	toolBar->addSeparator();
	toolBar->addWidget(_samplingRateCombo);
	addToolBar(toolBar);

	const auto centralWidget = new QWidget{ this };
	setCentralWidget(centralWidget);

	const auto rootLayout = new QHBoxLayout{ centralWidget };
	rootLayout->setContentsMargins({});
	rootLayout->setSpacing(0);

	_voiceWidget = new VoiceWidget{ centralWidget };
	_voiceWidget->setSizePolicy(::makeExpandingSizePolicy(0, 0));
	rootLayout->addWidget(_voiceWidget);
	connect(_voiceWidget, &VoiceWidget::voiceChanged, [this] {
		_changed = true;
		updateStatus();
	});

	const auto splitter = new QSplitter{ Qt::Vertical, this };
	splitter->setChildrenCollapsible(false);
	splitter->setSizePolicy(::makeExpandingSizePolicy(1, 1));
	rootLayout->addWidget(splitter);

	_compositionWidget = new CompositionWidget{ splitter };
	splitter->addWidget(_compositionWidget);

	const auto sequenceWrapper = new QWidget{ splitter };
	splitter->addWidget(sequenceWrapper);

	const auto sequenceLayout = new QGridLayout{ sequenceWrapper };
	sequenceLayout->setContentsMargins({});

	_sequenceWidget = new SequenceWidget{ sequenceWrapper };
	sequenceLayout->addWidget(_sequenceWidget, 0, 0, 1, 2);

	_autoRepeatButton = new QPushButton{ this };
	_autoRepeatButton->setCheckable(true);
	_autoRepeatButton->setIcon(qApp->style()->standardIcon(QStyle::SP_BrowserReload));
	_autoRepeatButton->setText(tr("Auto-repeat"));
	sequenceLayout->addWidget(_autoRepeatButton, 1, 0);
	connect(_autoRepeatButton, &QPushButton::toggled, [this](bool checked) {
		if (!checked)
			_autoRepeatNote.reset();
	});

	sequenceLayout->addItem(new QSpacerItem{ 0, 0, QSizePolicy::Expanding, QSizePolicy::Minimum }, 1, 1);

	splitter->setSizes({ 1, 1 });

	_statusPath = new QLabel{ statusBar() };
	_statusPath->setTextFormat(Qt::RichText);
	statusBar()->addWidget(_statusPath);

	connect(_speedSpin, QOverload<int>::of(&QSpinBox::valueChanged), [this] {
		if (!_hasComposition)
			return;
		_composition->_speed = static_cast<unsigned>(_speedSpin->value());
		_compositionWidget->setSpeed(_composition->_speed);
		_changed = true;
		updateStatus();
	});
	connect(_player.get(), &Player::stateChanged, [this] {
		if (_mode == Mode::Editing)
		{
			if (!_player->isPlaying() && _autoRepeatNote)
			{
				assert(_autoRepeatButton->isChecked());
				playNote(*_autoRepeatNote);
			}
			return;
		}
		assert(_mode == Mode::Playing);
		_compositionWidget->showCursor(_player->isPlaying());
		if (!_player->isPlaying())
			_mode = Mode::Editing;
		updateStatus();
	});
	connect(_player.get(), &Player::timeAdvanced, [this](qint64 microseconds) {
		if (_mode != Mode::Playing)
			return;
		_compositionWidget->setPlaybackOffset(microseconds * _composition->_speed / 1'000'000.0);
	});
	connect(_compositionWidget, &CompositionWidget::selectionChanged, [this](const std::shared_ptr<aulos::VoiceData>& voice, const std::shared_ptr<aulos::SequenceData>& sequence) {
		_voiceWidget->setVoice(voice);
		_sequenceWidget->setSequence(sequence);
		_autoRepeatButton->setChecked(false);
		updateStatus();
	});
	connect(_compositionWidget, &CompositionWidget::compositionChanged, [this] {
		_autoRepeatButton->setChecked(false);
		_changed = true;
		updateStatus();
	});
	connect(_sequenceWidget, &SequenceWidget::noteActivated, [this](aulos::Note note) {
		bool play = true;
		if (_autoRepeatButton->isChecked())
		{
			play = !_autoRepeatNote.has_value();
			_autoRepeatNote = note;
		}
		if (play)
			playNote(note);
	});
	connect(_sequenceWidget, &SequenceWidget::sequenceChanged, [this] {
		_compositionWidget->updateSelectedSequence(_sequenceWidget->sequence());
		_autoRepeatButton->setChecked(false);
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
	_compositionWidget->setComposition({});
	_player->stop();
	_mode = Mode::Editing;
}

void Studio::createEmptyComposition()
{
	assert(!_hasComposition);
	assert(_compositionPath.isEmpty());
	_composition = std::make_shared<aulos::CompositionData>();
	_composition->_speed = 6;
	const auto& part = _composition->_parts.emplace_back(std::make_shared<aulos::PartData>(::makeDefaultVoice()));
	part->_voiceName = tr("NewVoice").toStdString();
	part->_tracks.emplace_back(std::make_shared<aulos::TrackData>(1));
	_compositionFileName = tr("New composition");
	_speedSpin->setValue(static_cast<int>(_composition->_speed));
	_compositionWidget->setComposition(_composition);
	_hasComposition = true;
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

	const auto samplingRate = _samplingRateCombo->currentData().toUInt();
	const auto channels = _channelsCombo->currentData().toUInt();

	QByteArray rawData;
	Player::renderData(rawData, *aulos::Renderer::create(*composition, samplingRate, channels));

	constexpr size_t chunkHeaderSize = 8;
	constexpr size_t fmtChunkSize = 16;
	constexpr auto totalHeadersSize = chunkHeaderSize + 4 + chunkHeaderSize + fmtChunkSize + chunkHeaderSize;

	file.write("RIFF");
	::writeValue<uint32_t>(file, totalHeadersSize + rawData.size());
	assert(file.pos() == chunkHeaderSize);
	file.write("WAVE");
	file.write("fmt ");
	::writeValue<uint32_t>(file, fmtChunkSize);
	::writeValue<uint16_t>(file, 3);                                               // Data format: IEEE float PCM samples.
	::writeValue<uint16_t>(file, channels);                                        // Channels.
	::writeValue<uint32_t>(file, samplingRate);                                    // Samples per second.
	::writeValue<uint32_t>(file, samplingRate * channels * sizeof(float));         // Bytes per second.
	::writeValue<uint16_t>(file, static_cast<uint16_t>(channels * sizeof(float))); // Bytes per frame.
	::writeValue<uint16_t>(file, sizeof(float) * 8);                               // Bits per sample.
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
	_compositionWidget->setComposition(_composition);
	_hasComposition = true;
	setRecentFile(path);
	saveRecentFiles();
	return true;
}

void Studio::playNote(aulos::Note note)
{
	const auto voice = _voiceWidget->voice();
	assert(voice);
	const auto renderer = aulos::VoiceRenderer::create(*voice, _samplingRateCombo->currentData().toUInt(), _channelsCombo->currentData().toUInt());
	assert(renderer);
	renderer->start(note, _compositionWidget->selectedTrackWeight());
	_player->reset(*renderer);
	_player->start();
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
	if (const auto i = std::find_if(_recentFilesActions.begin(), _recentFilesActions.end(), [&path](const QAction* action) { return action->text() == path; }); i != _recentFilesActions.end())
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

void Studio::saveRecentFiles() const
{
	QStringList recentFiles;
	for (const auto action : _recentFilesActions)
		recentFiles << action->text();
	::saveRecentFileList(recentFiles);
}

void Studio::updateStatus()
{
	const auto applicationName = QCoreApplication::applicationName() + ' ' + QCoreApplication::applicationVersion();
	const auto compositionName = _hasComposition && !_composition->_title.empty() ? QString::fromStdString(_composition->_title) : _compositionFileName;
	setWindowTitle(_hasComposition ? QStringLiteral("%1 - %2").arg(_changed ? '*' + compositionName : compositionName, applicationName) : applicationName);
	_fileSaveAction->setEnabled(_changed);
	_fileSaveAsAction->setEnabled(_hasComposition);
	_fileExportAction->setEnabled(_hasComposition);
	_fileCloseAction->setEnabled(_hasComposition);
	_editInfoAction->setEnabled(_hasComposition);
	_playAction->setEnabled(_hasComposition && _mode == Mode::Editing);
	_stopAction->setEnabled(_hasComposition && _mode == Mode::Playing);
	_speedSpin->setEnabled(_hasComposition && _mode == Mode::Editing);
	_channelsCombo->setEnabled(_hasComposition && _mode == Mode::Editing);
	_samplingRateCombo->setEnabled(_hasComposition && _mode == Mode::Editing);
	_compositionWidget->setInteractive(_hasComposition && _mode == Mode::Editing);
	_voiceWidget->setEnabled(_hasComposition && _mode == Mode::Editing && _voiceWidget->voice());
	_sequenceWidget->setInteractive(_hasComposition && _mode == Mode::Editing && _voiceWidget->voice());
	_autoRepeatButton->setEnabled(_hasComposition && _mode == Mode::Editing && _voiceWidget->voice());
	if (!_autoRepeatButton->isEnabled())
		_autoRepeatButton->setChecked(false);
	_statusPath->setText(_compositionPath.isEmpty() ? QStringLiteral("<i>%1</i>").arg(tr("No file")) : _compositionPath);
}

void Studio::closeEvent(QCloseEvent* e)
{
	e->setAccepted(maybeSaveComposition());
}
