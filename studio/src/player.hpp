// This file is part of the Aulos toolkit.
// Copyright (C) Sergei Blagodarin.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <memory>

#include <QTimer>

#ifdef Q_OS_WIN
#	include <aulosplay/player.hpp>
#else
class QAudioOutput;
#endif

namespace aulos
{
	class Renderer;
}

class AudioSource;

class Player final
	: public QObject
#ifdef Q_OS_WIN
	, private aulosplay::PlayerCallbacks
#endif
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
#ifdef Q_OS_WIN
	void playbackError(const QString&);
	void playbackStarted();
	void playbackStopped();
#endif
	void stateChanged();

private:
#ifdef Q_OS_WIN
	void onPlaybackError(std::string&& message) override;
	void onPlaybackStarted() override;
	void onPlaybackStopped() override;
#endif

private:
	enum class State
	{
		Stopped,
		Started,
	};

#ifdef Q_OS_WIN
	QTimer _timer;
	std::shared_ptr<class AudioSource> _source;
	std::unique_ptr<aulosplay::Player> _backend;
#else
	std::unique_ptr<AudioSource> _source;
	std::unique_ptr<QAudioOutput> _output;
	double _samplingRate = 0;
	qint64 _lastProcessedUs = 0;
	double _currentOffset = 0;
#endif
	State _state = State::Stopped;
};
