// This file is part of the Aulos toolkit.
// Copyright (C) Sergei Blagodarin.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <memory>

#include <QObject>

#if QT_VERSION_MAJOR == 5
class QAudioOutput;
#endif

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

#if QT_VERSION_MAJOR == 5
	std::unique_ptr<AudioSource> _source;
	std::unique_ptr<QAudioOutput> _output;
	double _samplingRate = 0;
	qint64 _lastProcessedUs = 0;
#endif
	State _state = State::Stopped;
	double _currentOffset = 0;
};
