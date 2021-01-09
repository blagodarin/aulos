// This file is part of the Aulos toolkit.
// Copyright (C) Sergei Blagodarin.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <memory>

#include <QObject>

#ifdef Q_OS_WIN
#	include "player_wasapi.hpp"
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
	, private PlayerCallbacks
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
	void onPlayerError(std::string_view api, uintptr_t code, const std::string& description) override;
	void onPlayerStarted() override;
	void onPlayerStopped() override;
#endif

private:
	enum class State
	{
		Stopped,
		Started,
	};

#ifdef Q_OS_WIN
	std::unique_ptr<class PlayerBackend> _backend;
#else
	std::unique_ptr<AudioSource> _source;
	std::unique_ptr<QAudioOutput> _output;
	double _samplingRate = 0;
	qint64 _lastProcessedUs = 0;
#endif
	State _state = State::Stopped;
	double _currentOffset = 0;
};
