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

#include <cstring>

#include <QAudioOutput>

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
		_state = state == QAudio::ActiveState ? State::Started : State::Stopped;
		emit stateChanged();
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
	data.clear();
	data.resize(65'536);
	for (int totalRendered = 0;;)
	{
		std::memset(data.data() + totalRendered, 0, data.size() - totalRendered);
		if (const auto rendered = renderer.render(data.data() + totalRendered, data.size() - totalRendered); rendered > 0)
		{
			totalRendered += static_cast<int>(rendered);
			data.resize(totalRendered + 65'536);
		}
		else
		{
			data.resize(totalRendered);
			break;
		}
	}
}
