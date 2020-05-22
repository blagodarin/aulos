// This file is part of the Aulos toolkit.
// Copyright (C) Sergei Blagodarin.
// SPDX-License-Identifier: Apache-2.0

#include "player.hpp"

#include <aulos/composition.hpp>

#include <cassert>
#include <cstring>

#include <QAudioOutput>

class AudioSource final : public QIODevice
{
public:
	AudioSource(std::unique_ptr<aulos::Renderer>&& renderer, size_t minBufferBytes)
		: _renderer{ std::move(renderer) }
		, _minRemainingBytes{ minBufferBytes }
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
		const auto bufferBytes = static_cast<size_t>(maxSize);
		std::memset(data, 0, bufferBytes); // Single-voice renderers don't clear buffers.
		auto renderedBytes = _renderer->render(data, bufferBytes);
		assert(renderedBytes <= bufferBytes);
		_minRemainingBytes -= std::min(_minRemainingBytes, renderedBytes);
		if (renderedBytes < bufferBytes && _minRemainingBytes > 0)
		{
			const auto paddingBytes = std::min(bufferBytes - renderedBytes, _minRemainingBytes);
			renderedBytes += paddingBytes;
			_minRemainingBytes -= paddingBytes;
		}
		return renderedBytes;
	}

	qint64 writeData(const char*, qint64) override
	{
		return -1;
	}

private:
	std::unique_ptr<aulos::Renderer> _renderer;
	size_t _minRemainingBytes;
};

Player::Player(QObject* parent)
	: QObject{ parent }
{
}

Player::~Player() = default;

void Player::start(std::unique_ptr<aulos::Renderer>&& renderer, size_t minBufferBytes)
{
	stop();
	QAudioFormat format;
	format.setByteOrder(QAudioFormat::LittleEndian);
	format.setChannelCount(renderer->channels());
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
	_source = std::make_unique<AudioSource>(std::move(renderer), minBufferBytes);
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

void Player::renderData(QByteArray& data, aulos::Renderer& renderer)
{
	const auto totalBytes = renderer.totalBytes();
	data.resize(static_cast<int>(totalBytes));
	data.fill(0);
	data.resize(static_cast<int>(renderer.render(data.data(), totalBytes)));
}
