// This file is part of the Aulos toolkit.
// Copyright (C) Sergei Blagodarin.
// SPDX-License-Identifier: Apache-2.0

#include "player.hpp"

#include <seir_audio/decoder.hpp>
#include <seir_synth/format.hpp>
#include <seir_synth/renderer.hpp>

#include <cstring>
#include <mutex>

#include <QDebug>

class AudioDecoder final : public seir::AudioDecoder
{
public:
	AudioDecoder(std::unique_ptr<seir::synth::Renderer>&& renderer, size_t minBufferFrames)
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
			_format.channelLayout() == seir::synth::ChannelLayout::Stereo ? seir::AudioChannelLayout::Stereo : seir::AudioChannelLayout::Mono,
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

	bool seek(size_t frameOffset) override
	{
		_renderer->restart();
		return _renderer->skipFrames(frameOffset) == frameOffset;
	}

private:
	mutable std::mutex _mutex;
	const std::unique_ptr<seir::synth::Renderer> _renderer;
	const seir::synth::AudioFormat _format = _renderer->format();
	size_t _minRemainingFrames = 0;
};

Player::Player(QObject* parent)
	: QObject{ parent }
	, _backend{ seir::AudioPlayer::create(*this) }
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
		emit offsetChanged(static_cast<double>(_decoder->currentOffset()));
	});
}

Player::~Player() = default;

void Player::start(std::unique_ptr<seir::synth::Renderer>&& renderer, [[maybe_unused]] size_t minBufferFrames)
{
	stop();
	_decoder = seir::makeShared<AudioDecoder>(std::move(renderer), minBufferFrames);
	emit offsetChanged(static_cast<double>(_decoder->currentOffset()));
	_backend->play(seir::SharedPtr<seir::AudioDecoder>{ _decoder });
}

void Player::stop()
{
	_backend->stopAll();
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
