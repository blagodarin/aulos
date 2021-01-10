// This file is part of the Aulos toolkit.
// Copyright (C) Sergei Blagodarin.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <atomic>
#include <memory>
#include <mutex>
#include <string_view>
#include <thread>

class PlayerCallbacks
{
public:
	virtual ~PlayerCallbacks() noexcept = default;

	virtual void onPlaybackError(std::string_view api, uintptr_t code, const std::string& description) = 0;
	virtual void onPlaybackStarted() = 0;
	virtual void onPlaybackStopped() = 0;
};

class PlayerSource
{
public:
	virtual ~PlayerSource() noexcept = default;

	virtual size_t onRead(float* buffer, size_t maxFrames) noexcept = 0;
};

class PlayerBackend
{
public:
	PlayerBackend(PlayerCallbacks&, unsigned samplingRate);
	~PlayerBackend() noexcept;

	[[nodiscard]] size_t currentOffset() const noexcept { return _offset.load(); }
	void play(const std::shared_ptr<PlayerSource>& source);
	[[nodiscard]] constexpr unsigned samplingRate() const noexcept { return _samplingRate; }
	void stop();

private:
	PlayerCallbacks& _callbacks;
	const unsigned _samplingRate;
	std::atomic<size_t> _offset{ 0 };
	std::atomic<bool> _stop{ false };
	std::shared_ptr<PlayerSource> _source;
	bool _sourceChanged = false;
	std::mutex _mutex;
	std::condition_variable _condition;
	std::thread _thread;
};
