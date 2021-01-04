// This file is part of the Aulos toolkit.
// Copyright (C) Sergei Blagodarin.
// SPDX-License-Identifier: Apache-2.0

#include "studio.hpp"

#include "composition/composition_widget.hpp"
#include "sequence/sequence_widget.hpp"
#include "info_editor.hpp"
#include "player.hpp"
#include "theme.hpp"
#include "voice_widget.hpp"

#include <aulos/composition.hpp>
#include <aulos/renderer.hpp>

#include <cassert>
#include <stdexcept>

#include <QApplication>
#include <QCheckBox>
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
	constexpr size_t kBufferSize = 8192;
	constexpr int kMaxRecentFiles = 10;
	const auto kRecentFileKeyBase = QStringLiteral("RecentFile%1");

	std::unique_ptr<aulos::Composition> packComposition(aulos::CompositionData& data)
	{
		const auto maxAmplitude = [](const aulos::Composition& composition) {
			// TODO: Implement gain calculation with looping.
			const auto renderer = aulos::Renderer::create(composition, { aulos::Renderer::kMaxSamplingRate, aulos::ChannelLayout::Mono }, false);
			assert(renderer);
			float minimum = 0.f;
			float maximum = 0.f;
			for (std::array<float, kBufferSize / sizeof(float)> buffer;;)
			{
				const auto framesRendered = renderer->render(buffer.data(), buffer.size());
				if (!framesRendered)
					break;
				const auto minmax = std::minmax_element(buffer.cbegin(), buffer.cbegin() + framesRendered);
				minimum = std::min(minimum, *minmax.first);
				maximum = std::max(maximum, *minmax.second);
			}
			return std::max(-minimum, maximum);
		};

		data._gainDivisor = 1;
		auto composition = data.pack();
		if (!composition)
			return {};
		data._gainDivisor = maxAmplitude(*composition);
		return data.pack();
	}

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
			{ 100ms, 1.f },
			{ 400ms, .5f },
			{ 500ms, 0.f },
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
		const auto composition = ::packComposition(*_composition);
		if (!composition)
			return;
		assert(_mode == Mode::Editing);
		_autoRepeatButton->setChecked(false);
		const auto format = selectedFormat();
		auto renderer = aulos::Renderer::create(*composition, format, _loopPlaybackCheck->isChecked());
		assert(renderer);
		renderer->skipFrames(_compositionWidget->startOffset() * format.samplingRate() / _composition->_speed);
		_player->stop();
		_mode = Mode::Playing;
		_player->start(std::move(renderer), 0);
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

	_channelLayoutCombo = new QComboBox{ this };
	_channelLayoutCombo->addItem(tr("Stereo"), static_cast<int>(aulos::ChannelLayout::Stereo));
	_channelLayoutCombo->addItem(tr("Mono"), static_cast<int>(aulos::ChannelLayout::Mono));

	_samplingRateCombo = new QComboBox{ this };
	const auto hz = tr("%L1 Hz");
	for (const auto samplingRate : { 48'000u, 44'100u, 32'000u, 24'000u, 22'050u, 16'000u, 11'025u, 8'000u })
		_samplingRateCombo->addItem(hz.arg(samplingRate), samplingRate);

	_loopPlaybackCheck = new QCheckBox{ tr("Loop playback"), this };

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
	toolBar->addWidget(_channelLayoutCombo);
	toolBar->addSeparator();
	toolBar->addWidget(_samplingRateCombo);
	toolBar->addSeparator();
	toolBar->addWidget(_loopPlaybackCheck);
	addToolBar(toolBar);

	const auto centralWidget = new QWidget{ this };
	setCentralWidget(centralWidget);

	const auto rootLayout = new QHBoxLayout{ centralWidget };
	rootLayout->setContentsMargins({});
	rootLayout->setSpacing(0);

	_voiceWidget = new VoiceWidget{ centralWidget };
	_voiceWidget->setSizePolicy(::makeExpandingSizePolicy(0, 0));
	rootLayout->addWidget(_voiceWidget);
	connect(_voiceWidget, &VoiceWidget::trackPropertiesChanged, [this] {
		_changed = true;
		updateStatus();
	});
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
	connect(_player.get(), &Player::offsetChanged, [this](double currentFrame) {
		if (_mode == Mode::Playing)
			_compositionWidget->setPlaybackOffset(currentFrame * _composition->_speed / _samplingRateCombo->currentData().toDouble());
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
	connect(_compositionWidget, &CompositionWidget::selectionChanged, [this](const std::shared_ptr<aulos::VoiceData>& voice, const std::shared_ptr<aulos::TrackData>& track, const std::shared_ptr<aulos::SequenceData>& sequence) {
		_voiceWidget->setParameters(voice, track ? track->_properties : nullptr);
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
	_loopPlaybackCheck->setChecked(false);
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
	part->_tracks.emplace_back(std::make_shared<aulos::TrackData>(std::make_shared<aulos::TrackProperties>()));
	_compositionFileName = tr("New composition");
	_speedSpin->setValue(static_cast<int>(_composition->_speed));
	_compositionWidget->setComposition(_composition);
	_hasComposition = true;
}

void Studio::exportComposition()
{
	const auto composition = ::packComposition(*_composition);
	if (!composition)
		return;

	const auto path = QFileDialog::getSaveFileName(this, tr("Export Composition"), {}, tr("WAV Files (*.wav)"));
	if (path.isNull())
		return;

	QSaveFile file{ path };
	if (!file.open(QIODevice::WriteOnly))
		return;

	const auto format = selectedFormat();
	const auto renderer = aulos::Renderer::create(*composition, format, false);
	assert(renderer);

	constexpr size_t chunkHeaderSize = 8;
	constexpr size_t fmtChunkSize = 16;
	constexpr auto totalHeadersSize = chunkHeaderSize + 4 + chunkHeaderSize + fmtChunkSize + chunkHeaderSize;

	file.write("RIFF");
	const auto riffSizePos = file.pos();
	::writeValue<uint32_t>(file, static_cast<uint32_t>(totalHeadersSize));
	assert(file.pos() == chunkHeaderSize);
	file.write("WAVE");
	file.write("fmt ");
	::writeValue<uint32_t>(file, fmtChunkSize);
	::writeValue<uint16_t>(file, 3); // Data format: IEEE float PCM samples.
	::writeValue<uint16_t>(file, format.channelCount());
	::writeValue<uint32_t>(file, format.samplingRate());
	::writeValue<uint32_t>(file, format.samplingRate() * format.bytesPerFrame());
	::writeValue<uint16_t>(file, format.bytesPerFrame());
	::writeValue<uint16_t>(file, sizeof(float) * 8);
	file.write("data");
	const auto dataSizePos = file.pos();
	::writeValue<uint32_t>(file, 0);
	assert(file.pos() == totalHeadersSize);

	size_t dataSize = 0;
	for (std::array<char, kBufferSize> buffer;;)
	{
		auto renderedBytes = renderer->render(reinterpret_cast<float*>(buffer.data()), buffer.size() / format.bytesPerFrame()) * format.bytesPerFrame();
		if (!renderedBytes)
			break;
		file.write(buffer.data(), static_cast<qint64>(renderedBytes));
		dataSize += renderedBytes;
	}

	file.seek(riffSizePos);
	::writeValue<uint32_t>(file, static_cast<uint32_t>(totalHeadersSize + dataSize));

	file.seek(dataSizePos);
	::writeValue<uint32_t>(file, static_cast<uint32_t>(dataSize));

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
	const auto format = selectedFormat();
	_player->start(aulos::Renderer::create(*aulos::CompositionData{ _voiceWidget->voice(), note }.pack(), format), format.samplingRate());
}

bool Studio::saveComposition(const QString& path) const
{
	assert(_hasComposition);
	assert(!path.isEmpty());
	const auto composition = ::packComposition(*_composition);
	assert(composition);
	const auto buffer = aulos::serialize(*composition);
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

aulos::AudioFormat Studio::selectedFormat() const
{
	return { _samplingRateCombo->currentData().toUInt(), static_cast<aulos::ChannelLayout>(_channelLayoutCombo->currentData().toInt()) };
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
	_channelLayoutCombo->setEnabled(_hasComposition && _mode == Mode::Editing);
	_samplingRateCombo->setEnabled(_hasComposition && _mode == Mode::Editing);
	_loopPlaybackCheck->setEnabled(_hasComposition && _mode == Mode::Editing);
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
