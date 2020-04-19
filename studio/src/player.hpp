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

#pragma once

#include <memory>

#include <QByteArray>
#include <QBuffer>
#include <QObject>

class QAudioOutput;

namespace aulos
{
	class Renderer;
}

class Player final : public QObject
{
	Q_OBJECT

public:
	Player(QObject* parent = nullptr);
	~Player() override;

	constexpr bool isPlaying() const noexcept { return _state == State::Starting || _state == State::Started; }
	void reset(aulos::Renderer&);
	void start();
	void stop();

	static void renderData(QByteArray&, aulos::Renderer&);

signals:
	void stateChanged();
	void timeAdvanced(qint64 microseconds);

private:
	enum class State
	{
		Stopped,
		Starting,
		Started,
	};

	QByteArray _data;
	QBuffer _buffer;
	std::unique_ptr<QAudioOutput> _output;
	State _state = State::Stopped;
};
