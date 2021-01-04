// This file is part of the Aulos toolkit.
// Copyright (C) Sergei Blagodarin.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <memory>

#include <QObject>

class QAudioOutput;

namespace aulos
{
	class Renderer;
}

class AudioSource;

class Player final : public QObject
{
	Q_OBJECT

public:
	explicit Player(QObject* parent = nullptr);
	~Player() override;

	constexpr bool isPlaying() const noexcept { return _state == State::Started; }
	void start(std::unique_ptr<aulos::Renderer>&&, size_t minBufferBytes);
	void stop();

signals:
	void offsetChanged(double currentFrame);
	void stateChanged();

private:
	enum class State
	{
		Stopped,
		Started,
	};

	std::unique_ptr<AudioSource> _source;
	std::unique_ptr<QAudioOutput> _output;
	State _state = State::Stopped;
	double _samplingRate = 0;
	double _currentOffset = 0;
	qint64 _lastProcessedUs = 0;
};
