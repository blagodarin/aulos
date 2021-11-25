// This file is part of the Aulos toolkit.
// Copyright (C) Sergei Blagodarin.
// SPDX-License-Identifier: Apache-2.0

#include "player.hpp"

#include <aulos/format.hpp>
#include <aulos/renderer.hpp>

#include <cstring>
#include <mutex>

#include <QDebug>

class AudioSource final : public seir::AudioSource
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
	seir::AudioFormat format() const noexcept override
	{
		return {
			seir::AudioSampleType::f32,
			_format.channelLayout() == aulos::ChannelLayout::Stereo ? seir::AudioChannelLayout::Stereo : seir::AudioChannelLayout::Mono,
			_format.samplingRate()
		};
	}

	size_t read(void* buffer, size_t maxFrames) noexcept override
	{
		std::unique_lock lock{ _mutex };
		auto renderedFrames = _renderer->render(static_cast<float*>(buffer), maxFrames);
		lock.unlock();
		_minRemainingFrames -= std::min(_minRemainingFrames, renderedFrames);
		if (renderedFrames < maxFrames && _minRemainingFrames > 0)
		{
			const auto paddingFrames = std::min(maxFrames - renderedFrames, _minRemainingFrames);
			std::memset(static_cast<float*>(buffer) + renderedFrames * _format.channelCount(), 0, paddingFrames * _format.bytesPerFrame());
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
	connect(this, &Player::playbackStarted, this, [this] {
		_state = State::Started;
		_timer.start();
		emit stateChanged();
	});
	connect(this, &Player::playbackStopped, this, [this] {
		if (_state != State::Stopped)
		{
			_state = State::Stopped;
			_timer.stop();
			emit stateChanged();
		}
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
	if (!_backend || _backend->format().samplingRate() != samplingRate)
	{
		_backend.reset();
		_backend = seir::AudioPlayer::create(*this, samplingRate);
	}
	_source = seir::makeShared<AudioSource>(std::move(renderer), minBufferFrames);
	emit offsetChanged(static_cast<double>(_source->currentOffset()));
	_backend->play(seir::SharedPtr<seir::AudioSource>{ _source });
}

void Player::stop()
{
	if (_backend)
		_backend->stop();
}

void Player::onPlaybackError(seir::AudioError error)
{
	switch (error)
	{
	case seir::AudioError::NoDevice: qWarning() << "seir::AudioError::NoDevice"; break;
	}
	emit playbackStopped();
}

void Player::onPlaybackError(std::string&& message)
{
	qWarning() << QString::fromStdString(message);
	emit playbackStopped();
}

void Player::onPlaybackStarted()
{
	emit playbackStarted();
}

void Player::onPlaybackStopped()
{
	emit playbackStopped();
}
