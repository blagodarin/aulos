// This file is part of the Aulos toolkit.
// Copyright (C) Sergei Blagodarin.
// SPDX-License-Identifier: Apache-2.0

#include "player.hpp"

#include <aulos/composition.hpp>

#include <cassert>
#include <cstring>

#include <QAudioOutput>
#include <QDebug>

Player::Player(QObject* parent)
	: QObject{ parent }
	, _buffer{ &_data }
{
}

Player::~Player() = default;

void Player::reset(aulos::Renderer& renderer, int minBufferBytes)
{
	stop();
	_output.reset();
	_buffer.close();
	renderData(_data, renderer);
	if (const auto size = _data.size(); size < minBufferBytes)
	{
		_data.resize(minBufferBytes);
		std::memset(_data.data() + size, 0, _data.size() - size);
	}
	_buffer.open(QIODevice::ReadOnly);
	QAudioFormat format;
	format.setByteOrder(QAudioFormat::LittleEndian);
	format.setChannelCount(renderer.channels());
	format.setCodec("audio/pcm");
	format.setSampleRate(renderer.samplingRate());
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
}

void Player::start()
{
	if (!_buffer.isOpen() || _state != State::Stopped)
		return;
	_buffer.seek(0);
	_output->start(&_buffer);
	emit timeAdvanced(0);
}

void Player::stop()
{
	if (!_buffer.isOpen())
		return;
	_output->stop();
}

void Player::renderData(QByteArray& data, aulos::Renderer& renderer)
{
	const auto totalBytes = renderer.totalBytes();
	data.resize(static_cast<int>(totalBytes));
	data.fill(0);
	data.resize(static_cast<int>(renderer.render(data.data(), totalBytes)));
}
