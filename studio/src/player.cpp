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

#include "player.hpp"

#include <aulos/playback.hpp>

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

void Player::reset(aulos::Renderer& renderer)
{
	stop();
	_output.reset();
	_buffer.close();
	renderData(_data, renderer);
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
