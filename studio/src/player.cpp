// This file is part of the Aulos toolkit.
// Copyright (C) Sergei Blagodarin.
// SPDX-License-Identifier: Apache-2.0

#include "player.hpp"

#include <aulos/composition.hpp>

#include <cassert>

#include <QAudioOutput>

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

private:
	qint64 readData(char* data, qint64 maxSize) override
	{
		const auto maxFrames = static_cast<size_t>(maxSize) / _renderer->bytesPerFrame();
		auto renderedFrames = _renderer->render(reinterpret_cast<float*>(data), maxFrames);
		assert(renderedFrames <= maxFrames);
		_minRemainingFrames -= std::min(_minRemainingFrames, renderedFrames);
		if (renderedFrames < maxFrames && _minRemainingFrames > 0)
		{
			const auto paddingFrames = std::min(maxFrames - renderedFrames, _minRemainingFrames);
			renderedFrames += paddingFrames;
			_minRemainingFrames -= paddingFrames;
		}
		return renderedFrames * _renderer->bytesPerFrame();
	}

	qint64 writeData(const char*, qint64) override
	{
		return -1;
	}

private:
	std::unique_ptr<aulos::Renderer> _renderer;
	size_t _minRemainingFrames;
};

Player::Player(QObject* parent)
	: QObject{ parent }
{
}

Player::~Player() = default;

void Player::start(std::unique_ptr<aulos::Renderer>&& renderer, size_t minBufferFrames)
{
	stop();
	QAudioFormat format;
	format.setByteOrder(QAudioFormat::LittleEndian);
	switch (renderer->channelLayout())
	{
	case aulos::ChannelLayout::Mono: format.setChannelCount(1); break;
	case aulos::ChannelLayout::Stereo: format.setChannelCount(2); break;
	}
	format.setCodec("audio/pcm");
	format.setSampleRate(renderer->samplingRate());
	format.setSampleSize(32);
	format.setSampleType(QAudioFormat::Float);
	_output = std::make_unique<QAudioOutput>(format);
	_output->setNotifyInterval(20);
	connect(_output.get(), &QAudioOutput::notify, this, [this] {
		emit timeAdvanced(_output->processedUSecs());
	});
	connect(_output.get(), &QAudioOutput::stateChanged, this, [this](QAudio::State state) {
		if (const auto newState = state == QAudio::ActiveState ? State::Started : State::Stopped; newState != _state)
		{
			_state = newState;
			emit stateChanged();
		}
	});
	_source = std::make_unique<AudioSource>(std::move(renderer), minBufferFrames);
	_output->start(_source.get());
	emit timeAdvanced(0);
}

void Player::stop()
{
	if (_source)
	{
		_output->stop();
		_output.reset();
		_source.reset();
	}
	assert(_state == State::Stopped);
}
