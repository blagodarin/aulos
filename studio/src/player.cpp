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

#include <aulos.hpp>

#include <QAudioOutput>

Player::Player(QObject* parent)
	: QObject{ parent }
	, _buffer{ &_data }
{
	QAudioFormat format;
	format.setByteOrder(QAudioFormat::LittleEndian);
	format.setChannelCount(1);
	format.setCodec("audio/pcm");
	format.setSampleRate(SamplingRate);
	format.setSampleSize(32);
	format.setSampleType(QAudioFormat::Float);
	_output = std::make_unique<QAudioOutput>(format);
	connect(_output.get(), &QAudioOutput::stateChanged, this, [this](QAudio::State state) {
		_state = state == QAudio::ActiveState ? State::Started : State::Stopped;
		emit stateChanged();
	});
}

void Player::reset(class aulos::Renderer& renderer)
{
	stop();
	_buffer.close();
	_data.clear();
	_data.resize(65'536);
	for (int totalRendered = 0;;)
	{
		std::memset(_data.data() + totalRendered, 0, _data.size() - totalRendered);
		if (const auto rendered = renderer.render(_data.data() + totalRendered, _data.size() - totalRendered); rendered > 0)
		{
			totalRendered += static_cast<int>(rendered);
			_data.resize(totalRendered + 65'536);
		}
		else
		{
			_data.resize(totalRendered);
			break;
		}
	}
	_buffer.open(QIODevice::ReadOnly);
}

void Player::start()
{
	if (!_buffer.isOpen() || _state != State::Stopped)
		return;
	_buffer.seek(0);
	_output->start(&_buffer);
}

void Player::stop()
{
	if (!_buffer.isOpen())
		return;
	_output->stop();
}
