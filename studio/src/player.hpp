// This file is part of the Aulos toolkit.
// Copyright (C) Sergei Blagodarin.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <seir_audio/player.hpp>

#include <memory>

#include <QTimer>

namespace seir::synth
{
	class Renderer;
}

class AudioDecoder;

class Player final
	: public QObject
	, private seir::AudioCallbacks
{
	Q_OBJECT

public:
	explicit Player(QObject* parent = nullptr);
	~Player() override;

	constexpr bool isPlaying() const noexcept { return _state == State::Started; }
	void start(std::unique_ptr<seir::synth::Renderer>&&, size_t minBufferBytes);
	void stop();

signals:
	void offsetChanged(double currentFrame);
	void playbackStarted();
	void playbackStopped();
	void stateChanged();

private:
	void onPlaybackError(seir::AudioError) override;
	void onPlaybackError(std::string&& message) override;
	void onPlaybackStarted() override;
	void onPlaybackStopped() override;

private:
	enum class State
	{
		Stopped,
		Started,
	};

	const seir::UniquePtr<seir::AudioPlayer> _backend;
	QTimer _timer;
	seir::SharedPtr<class AudioDecoder> _decoder;
	State _state = State::Stopped;
};
