//
// This file is part of the Aulos toolkit.
// Copyright (C) 2019 Sergei Blagodarin.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//

#include "playback_writer.hpp"

#include "main.hpp"

#include <array>
#include <stdexcept>
#include <vector>

#include <windows.h>
#include <mmreg.h>

#pragma comment(lib, "winmm.lib")

class PlaybackWriter : public Writer
{
public:
	PlaybackWriter()
	{
		for (auto& buffer : _buffers)
			_freeBuffers.emplace_back(&buffer);
	}

	~PlaybackWriter() noexcept override
	{
		for (auto& buffer : _buffers)
			if (buffer._header.dwFlags & WHDR_PREPARED)
				waveOutUnprepareHeader(_waveout, &buffer._header, sizeof buffer._header);
		if (_waveout)
			waveOutClose(_waveout);
		if (_event)
			CloseHandle(_event);
	}

	bool init(unsigned samplingRate, unsigned channels)
	{
		_event = CreateEventW(nullptr, TRUE, TRUE, nullptr);
		if (!_event)
			return false;

		WAVEFORMATEX format;
		format.wFormatTag = WAVE_FORMAT_IEEE_FLOAT;
		format.nChannels = 1;
		format.nSamplesPerSec = samplingRate;
		format.nAvgBytesPerSec = samplingRate * channels * sizeof(float);
		format.nBlockAlign = static_cast<WORD>(channels * sizeof(float));
		format.wBitsPerSample = sizeof(float) * 8;
		format.cbSize = 0;
		if (waveOutOpen(&_waveout, WAVE_MAPPER, &format, reinterpret_cast<DWORD_PTR>(_event), NULL, CALLBACK_EVENT) != MMSYSERR_NOERROR)
			return false;

		for (auto& buffer : _buffers)
			if (waveOutPrepareHeader(_waveout, &buffer._header, sizeof buffer._header) != MMSYSERR_NOERROR)
				return false;

		return true;
	}

	virtual void* buffer()
	{
		waitForBuffers(1);
		return _freeBuffers.back()->_data.data();
	}

	virtual size_t bufferSize() const noexcept
	{
		return playbackBufferSize;
	}

	void commit() override
	{
		waitForBuffers(_buffers.size());
	}

	void write(size_t size) override
	{
		auto& currentHeader = _freeBuffers.back()->_header;
		_freeBuffers.pop_back();
		currentHeader.dwBufferLength = static_cast<DWORD>(size);
		if (waveOutWrite(_waveout, &currentHeader, sizeof currentHeader) != MMSYSERR_NOERROR)
			throw std::runtime_error{ "Playback error" };
	}

private:
	void waitForBuffers(size_t count)
	{
		while (_freeBuffers.size() < count)
		{
			WaitForSingleObject(_event, INFINITE);
			ResetEvent(_event);
			for (auto& buffer : _buffers)
				if (buffer._header.dwFlags & WHDR_DONE)
					_freeBuffers.emplace_back(&buffer);
		}
	}

private:
	static constexpr DWORD playbackBufferSize = 8192;

	struct Buffer
	{
		WAVEHDR _header{};
		std::array<char, playbackBufferSize> _data;

		Buffer() noexcept
		{
			_header.lpData = _data.data();
			_header.dwBufferLength = playbackBufferSize;
		}
	};

	HANDLE _event = NULL;
	HWAVEOUT _waveout = NULL;
	std::vector<Buffer*> _freeBuffers;
	std::array<Buffer, 2> _buffers;
};

std::unique_ptr<Writer> makePlaybackWriter(unsigned samplingRate)
{
	auto writer = std::make_unique<PlaybackWriter>();
	if (!writer->init(samplingRate, 1))
		throw std::runtime_error{ "Failed to initialize playback" };
	return writer;
}
