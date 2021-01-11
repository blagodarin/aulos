// This file is part of the Aulos toolkit.
// Copyright (C) Sergei Blagodarin.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <aulosplay/player.hpp>

#include <QTimer>

namespace aulos
{
	class Renderer;
}

class AudioSource;

class Player final
	: public QObject
	, private aulosplay::PlayerCallbacks
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
	void playbackError(const QString&);
	void playbackStarted();
	void playbackStopped();
	void stateChanged();

private:
	void onPlaybackError(std::string&& message) override;
	void onPlaybackStarted() override;
	void onPlaybackStopped() override;

private:
	enum class State
	{
		Stopped,
		Started,
	};

	QTimer _timer;
	std::shared_ptr<class AudioSource> _source;
	std::unique_ptr<aulosplay::Player> _backend;
	State _state = State::Stopped;
};
