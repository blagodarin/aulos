// This file is part of the Aulos toolkit.
// Copyright (C) Sergei Blagodarin.
// SPDX-License-Identifier: Apache-2.0

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
	explicit Player(QObject* parent = nullptr);
	~Player() override;

	constexpr bool isPlaying() const noexcept { return _state == State::Started; }
	void reset(aulos::Renderer&, int minBufferBytes);
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
		Started,
	};

	QByteArray _data;
	QBuffer _buffer;
	std::unique_ptr<QAudioOutput> _output;
	State _state = State::Stopped;
};
