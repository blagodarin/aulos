// This file is part of the Aulos toolkit.
// Copyright (C) Sergei Blagodarin.
// SPDX-License-Identifier: Apache-2.0

#include "player.hpp"

#include <aulos/format.hpp>
#include <aulos/renderer.hpp>

#include <cassert>

#if QT_VERSION_MAJOR == 5
#	include <QAudioOutput>
#endif

#if QT_VERSION_MAJOR == 5
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
}

Player::~Player() = default;

void Player::start(std::unique_ptr<aulos::Renderer>&& renderer, [[maybe_unused]] size_t minBufferFrames)
{
	stop();
#if QT_VERSION_MAJOR == 5
	QAudioFormat format;
	format.setByteOrder(QAudioFormat::LittleEndian);
	format.setChannelCount(renderer->format().channelCount());
	format.setCodec("audio/pcm");
	format.setSampleRate(renderer->format().samplingRate());
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
#endif
	_currentOffset = renderer->currentOffset();
#if QT_VERSION_MAJOR == 5
	_samplingRate = format.sampleRate();
	_lastProcessedUs = 0;
	_source = std::make_unique<AudioSource>(std::move(renderer), minBufferFrames);
	_output->start(_source.get());
#endif
	emit offsetChanged(_currentOffset);
#if QT_VERSION_MAJOR == 6
	_state = State::Started;
	emit stateChanged();
#endif
}

void Player::stop()
{
#if QT_VERSION_MAJOR == 5
	if (_source)
	{
		_output->stop();
		_output.reset();
		_source.reset();
	}
	assert(_state == State::Stopped);
#else
	if (_state != State::Stopped)
	{
		_state = State::Stopped;
		emit stateChanged();
	}
#endif
}
