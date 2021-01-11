// This file is part of the Aulos toolkit.
// Copyright (C) Sergei Blagodarin.
// SPDX-License-Identifier: Apache-2.0

#include "player.hpp"

#include <aulos/format.hpp>
#include <aulos/renderer.hpp>

#include <mutex>

#include <QDebug>

class AudioSource final : public aulosplay::Source
{
public:
	AudioSource(std::unique_ptr<aulos::Renderer>&& renderer, size_t minBufferFrames)
		: _renderer{ std::move(renderer) }
		, _minRemainingFrames{ minBufferFrames }
	{
	}

	auto currentOffset() const noexcept
	{
		std::lock_guard lock{ _mutex };
		return _renderer->currentOffset();
	}

private:
	bool isStereo() const noexcept override
	{
		return _format.channelLayout() == aulos::ChannelLayout::Stereo;
	}

	size_t onRead(float* buffer, size_t maxFrames) noexcept override
	{
		std::unique_lock lock{ _mutex };
		auto renderedFrames = _renderer->render(buffer, maxFrames);
		lock.unlock();
		_minRemainingFrames -= std::min(_minRemainingFrames, renderedFrames);
		if (renderedFrames < maxFrames && _minRemainingFrames > 0)
		{
			const auto paddingFrames = std::min(maxFrames - renderedFrames, _minRemainingFrames);
			std::memset(buffer + renderedFrames * _format.channelCount(), 0, paddingFrames * _format.bytesPerFrame());
			renderedFrames += paddingFrames;
			_minRemainingFrames -= paddingFrames;
		}
		return renderedFrames;
	}

private:
	mutable std::mutex _mutex;
	const std::unique_ptr<aulos::Renderer> _renderer;
	const aulos::AudioFormat _format = _renderer->format();
	size_t _minRemainingFrames = 0;
};

Player::Player(QObject* parent)
	: QObject{ parent }
{
	_timer.setInterval(20);
	connect(this, &Player::playbackError, this, [this](const QString& error) {
		qInfo() << error;
		if (_state != State::Stopped)
		{
			_state = State::Stopped;
			_timer.stop();
			emit stateChanged();
		}
	});
	connect(this, &Player::playbackStarted, this, [this] {
		_state = State::Started;
		_timer.start();
		emit stateChanged();
	});
	connect(this, &Player::playbackStopped, this, [this] {
		_state = State::Stopped;
		_timer.stop();
		emit stateChanged();
	});
	connect(&_timer, &QTimer::timeout, this, [this] {
		emit offsetChanged(static_cast<double>(_source->currentOffset()));
	});
}

Player::~Player() = default;

void Player::start(std::unique_ptr<aulos::Renderer>&& renderer, [[maybe_unused]] size_t minBufferFrames)
{
	stop();
	const auto samplingRate = renderer->format().samplingRate();
	if (!_backend || _backend->samplingRate() != samplingRate)
	{
		_backend.reset();
		_backend = aulosplay::Player::create(*this, samplingRate);
	}
	_source = std::make_shared<AudioSource>(std::move(renderer), minBufferFrames);
	emit offsetChanged(static_cast<double>(_source->currentOffset()));
	_backend->play(_source);
}

void Player::stop()
{
	if (_backend)
		_backend->stop();
}

void Player::onPlaybackError(std::string&& message)
{
	emit playbackError(QString::fromStdString(message));
}

void Player::onPlaybackStarted()
{
	emit playbackStarted();
}

void Player::onPlaybackStopped()
{
	emit playbackStopped();
}
