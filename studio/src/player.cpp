// This file is part of the Aulos toolkit.
// Copyright (C) Sergei Blagodarin.
// SPDX-License-Identifier: Apache-2.0

#include "player.hpp"

#include <aulos/format.hpp>
#include <aulos/renderer.hpp>

#include <cassert>

#ifdef Q_OS_WIN
#	include <QDebug>
#else
#	include <QAudioOutput>
#endif

#ifdef Q_OS_WIN
class AudioSource final : public aulosplay::Source
{
public:
	AudioSource(std::unique_ptr<aulos::Renderer>&& renderer, size_t minBufferFrames)
		: _renderer{ std::move(renderer) }
		, _minRemainingFrames{ minBufferFrames }
	{
	}

	std::pair<size_t, size_t> loopRange() const
	{
		return { _renderer->loopOffset(), _loopEnd.load() };
	}

private:
	bool isStereo() const noexcept override
	{
		return _renderer->format().channelLayout() == aulos::ChannelLayout::Stereo;
	}

	size_t onRead(float* buffer, size_t maxFrames) noexcept override
	{
		const auto offsetBefore = _renderer->currentOffset();
		auto renderedFrames = _renderer->render(buffer, maxFrames);
		if (const auto offsetAfter = _renderer->currentOffset(); offsetAfter != offsetBefore + renderedFrames)
			_loopEnd = offsetBefore + renderedFrames - (offsetAfter - _renderer->loopOffset());
		_minRemainingFrames -= std::min(_minRemainingFrames, renderedFrames);
		if (renderedFrames < maxFrames && _minRemainingFrames > 0)
		{
			const auto format = _renderer->format();
			const auto paddingFrames = std::min(maxFrames - renderedFrames, _minRemainingFrames);
			std::memset(buffer + renderedFrames * format.channelCount(), 0, paddingFrames * format.bytesPerFrame());
			renderedFrames += paddingFrames;
			_minRemainingFrames -= paddingFrames;
		}
		return renderedFrames;
	}

private:
	std::unique_ptr<aulos::Renderer> _renderer;
	size_t _minRemainingFrames;
	std::atomic<size_t> _loopEnd{ std::numeric_limits<size_t>::max() };
};
#else
class AudioSource final : public QIODevice
{
public:
	AudioSource(std::unique_ptr<aulos::Renderer>&& renderer, size_t minBufferFrames)
		: _renderer{ std::move(renderer) }
		, _minRemainingFrames{ minBufferFrames }
	{
		open(QIODevice::ReadOnly | QIODevice::Unbuffered);
	}

	bool isSequential() const override
	{
		return true;
	}

	std::pair<double, double> loopRange() const
	{
		return { _renderer->loopOffset(), _loopEnd.load() };
	}

private:
	qint64 readData(char* data, qint64 maxSize) override
	{
		const auto bytesPerFrame = _renderer->format().bytesPerFrame();
		const auto maxFrames = static_cast<size_t>(maxSize / bytesPerFrame);
		const auto offsetBefore = _renderer->currentOffset();
		auto renderedFrames = _renderer->render(reinterpret_cast<float*>(data), maxFrames);
		assert(renderedFrames <= maxFrames);
		const auto offsetAfter = _renderer->currentOffset();
		if (offsetAfter != offsetBefore + renderedFrames)
			_loopEnd = offsetBefore + renderedFrames - (offsetAfter - _renderer->loopOffset());
		_minRemainingFrames -= std::min(_minRemainingFrames, renderedFrames);
		if (renderedFrames < maxFrames && _minRemainingFrames > 0)
		{
			const auto paddingFrames = std::min(maxFrames - renderedFrames, _minRemainingFrames);
			renderedFrames += paddingFrames;
			_minRemainingFrames -= paddingFrames;
		}
		return renderedFrames * bytesPerFrame;
	}

	qint64 writeData(const char*, qint64) override
	{
		return -1;
	}

private:
	std::unique_ptr<aulos::Renderer> _renderer;
	size_t _minRemainingFrames;
	std::atomic<size_t> _loopEnd{ std::numeric_limits<size_t>::max() };
};
#endif

Player::Player(QObject* parent)
	: QObject{ parent }
{
#ifdef Q_OS_WIN
	_timer.setInterval(20);
	connect(this, &Player::playbackError, this, [this](const QString& error) {
		qInfo() << error;
		if (_state != State::Stopped)
		{
			_state = State::Stopped;
			_timer.stop();
			emit stateChanged();
		}
	});
	connect(this, &Player::playbackStarted, this, [this] {
		_state = State::Started;
		_timer.start();
		emit stateChanged();
	});
	connect(this, &Player::playbackStopped, this, [this] {
		_state = State::Stopped;
		_timer.stop();
		emit stateChanged();
	});
	connect(&_timer, &QTimer::timeout, this, [this] {
		auto currentOffset = _startOffset + _backend->currentOffset();
		const auto loopRange = _source->loopRange();
		while (currentOffset > loopRange.second)
			currentOffset = loopRange.first + (currentOffset - loopRange.second);
		if (currentOffset != _lastOffset)
		{
			_lastOffset = currentOffset;
			emit offsetChanged(static_cast<double>(currentOffset));
		}
	});
#endif
}

Player::~Player() = default;

void Player::start(std::unique_ptr<aulos::Renderer>&& renderer, [[maybe_unused]] size_t minBufferFrames)
{
	stop();
	const auto samplingRate = renderer->format().samplingRate();
#ifdef Q_OS_WIN
	if (!_backend || _backend->samplingRate() != samplingRate)
	{
		_backend.reset();
		_backend = std::make_unique<aulosplay::Player>(static_cast<PlayerCallbacks&>(*this), samplingRate);
	}
	_startOffset = renderer->currentOffset();
	_lastOffset = _startOffset;
	_source = std::make_shared<AudioSource>(std::move(renderer), minBufferFrames);
	_backend->play(_source);
#else
	QAudioFormat format;
	format.setByteOrder(QAudioFormat::LittleEndian);
	format.setChannelCount(renderer->format().channelCount());
	format.setCodec("audio/pcm");
	format.setSampleRate(samplingRate);
	format.setSampleSize(32);
	format.setSampleType(QAudioFormat::Float);
	_output = std::make_unique<QAudioOutput>(format);
	_output->setNotifyInterval(20);
	connect(_output.get(), &QAudioOutput::notify, this, [this] {
		const auto advanceUs = _output->processedUSecs() - _lastProcessedUs;
		if (!advanceUs)
			return;
		_currentOffset += _samplingRate * advanceUs / 1'000'000.0;
		_lastProcessedUs += advanceUs;
		const auto loopRange = _source->loopRange();
		if (_currentOffset > loopRange.second)
			_currentOffset = loopRange.first + (_currentOffset - loopRange.second);
		emit offsetChanged(_currentOffset);
	});
	connect(_output.get(), &QAudioOutput::stateChanged, this, [this](QAudio::State state) {
		if (const auto newState = state == QAudio::ActiveState ? State::Started : State::Stopped; newState != _state)
		{
			_state = newState;
			emit stateChanged();
		}
	});
	_currentOffset = renderer->currentOffset();
	_samplingRate = samplingRate;
	_lastProcessedUs = 0;
	_source = std::make_unique<AudioSource>(std::move(renderer), minBufferFrames);
	_output->start(_source.get());
	emit offsetChanged(_currentOffset);
#endif
}

void Player::stop()
{
#ifdef Q_OS_WIN
	if (_backend)
		_backend->stop();
#else
	if (_source)
	{
		_output->stop();
		_output.reset();
		_source.reset();
	}
	assert(_state == State::Stopped);
#endif
}

#ifdef Q_OS_WIN
void Player::onPlaybackError(std::string_view api, uintptr_t code, const std::string& description)
{
	emit playbackError(description.empty()
			? QStringLiteral("[%1] Error 0x%2").arg(QString::fromStdString(std::string{ api })).arg(code, 8, 16, QLatin1Char{ '0' })
			: QStringLiteral("[%1] Error 0x%2: %3").arg(QString::fromStdString(std::string{ api })).arg(code, 8, 16, QLatin1Char{ '0' }).arg(QString::fromStdString(description)));
}

void Player::onPlaybackStarted()
{
	emit playbackStarted();
}

void Player::onPlaybackStopped()
{
	emit playbackStopped();
}
#endif
