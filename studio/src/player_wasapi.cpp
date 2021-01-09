// This file is part of the Aulos toolkit.
// Copyright (C) Sergei Blagodarin.
// SPDX-License-Identifier: Apache-2.0

#include "player_wasapi.hpp"

#include <numeric>
#include <string>

#include <audioclient.h>
#include <comdef.h>
#include <mmdeviceapi.h>

namespace
{
	constexpr auto kChannels = 2u;
	constexpr auto kSimdAlignment = 16u;
	constexpr auto kFrameBytes = kChannels * sizeof(float);
	constexpr auto kFrameAlignment = static_cast<UINT32>(std::lcm(kSimdAlignment, kFrameBytes) / kFrameBytes);

	template <typename T>
	struct ComBuffer
	{
		T* _data = nullptr;
		~ComBuffer() noexcept { ::CoTaskMemFree(_data); }
		T* operator->() noexcept { return _data; }
	};

	template <typename T>
	using ComPtr = _com_ptr_t<_com_IIID<T, &__uuidof(T)>>;

	struct ComUninitializer
	{
		~ComUninitializer() noexcept { ::CoUninitialize(); }
	};

	struct HandleWrapper
	{
		HANDLE _handle = NULL;
		~HandleWrapper() noexcept { ::CloseHandle(_handle); }
	};

	struct AudioClientStopper
	{
		IAudioClient* _audioClient = nullptr;

		~AudioClientStopper() noexcept
		{
			if (_audioClient)
				_audioClient->Stop();
		}
	};

	std::string errorToString(HRESULT error)
	{
		struct LocalBuffer
		{
			char* _data = nullptr;
			~LocalBuffer() noexcept { ::LocalFree(_data); }
		};

		LocalBuffer buffer;
		::FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
			nullptr, error, MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), buffer._data, 0, nullptr);
		if (!buffer._data)
			return {};
		auto length = std::strlen(buffer._data);
		if (length > 0 && buffer._data[length - 1] == '\n')
		{
			--length;
			if (length > 0 && buffer._data[length - 1] == '\r')
				--length;
			buffer._data[length] = '\0';
		}
		return buffer._data;
	}
}

PlayerBackend::PlayerBackend(PlayerCallbacks& callbacks, unsigned samplingRate)
	: _callbacks{ callbacks }
	, _samplingRate{ samplingRate }
{
	_thread = std::thread{ [this] {
		if (const auto hr = ::CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED); FAILED(hr))
			return _callbacks.onPlayerError("CoInitializeEx", hr, ::errorToString(hr));
		ComUninitializer comUninitializer;
		ComPtr<IMMDeviceEnumerator> deviceEnumerator;
		if (const auto hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), reinterpret_cast<void**>(&deviceEnumerator)); !deviceEnumerator)
			return _callbacks.onPlayerError("CoCreateInstance", hr, ::errorToString(hr));
		ComPtr<IMMDevice> device;
		if (const auto hr = deviceEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &device); !device)
			return _callbacks.onPlayerError("IMMDeviceEnumerator::GetDefaultAudioEndpoint", hr, ::errorToString(hr));
		ComPtr<IAudioClient> audioClient;
		if (const auto hr = device->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr, reinterpret_cast<void**>(&audioClient)); !audioClient)
			return _callbacks.onPlayerError("IMMDevice::Activate", hr, ::errorToString(hr));
		REFERENCE_TIME period = 0;
		if (const auto hr = audioClient->GetDevicePeriod(nullptr, &period); FAILED(hr))
			return _callbacks.onPlayerError("IAudioClient::GetDevicePeriod", hr, ::errorToString(hr));
		ComBuffer<WAVEFORMATEX> format;
		if (const auto hr = audioClient->GetMixFormat(&format._data); !format._data)
			return _callbacks.onPlayerError("IAudioClient::GetMixFormat", hr, ::errorToString(hr));
		if (format->wFormatTag == WAVE_FORMAT_EXTENSIBLE)
		{
			const auto extensible = reinterpret_cast<WAVEFORMATEXTENSIBLE*>(format._data);
			if (!::IsEqualGUID(extensible->SubFormat, KSDATAFORMAT_SUBTYPE_IEEE_FLOAT) || extensible->Format.wBitsPerSample != 32)
			{
				extensible->Format.wBitsPerSample = 32;
				extensible->Format.nBlockAlign = (format->wBitsPerSample / 8) * format->nChannels;
				extensible->Format.nAvgBytesPerSec = format->nBlockAlign * format->nSamplesPerSec;
				extensible->Samples.wValidBitsPerSample = 32;
				extensible->SubFormat = KSDATAFORMAT_SUBTYPE_IEEE_FLOAT;
			}
		}
		else if (format->wFormatTag != WAVE_FORMAT_IEEE_FLOAT || format->wBitsPerSample != 32)
		{
			format->wFormatTag = WAVE_FORMAT_IEEE_FLOAT;
			format->wBitsPerSample = 32;
			format->nBlockAlign = (format->wBitsPerSample / 8) * format->nChannels;
			format->nAvgBytesPerSec = format->nBlockAlign * format->nSamplesPerSec;
		}
		DWORD streamFlags = AUDCLNT_STREAMFLAGS_EVENTCALLBACK;
		if (format->nSamplesPerSec != _samplingRate)
		{
			streamFlags |= AUDCLNT_STREAMFLAGS_RATEADJUST;
			format->nSamplesPerSec = _samplingRate;
			format->nAvgBytesPerSec = format->nBlockAlign * format->nSamplesPerSec;
		}
		if (format->nChannels != kChannels)
		{
			format->nChannels = kChannels;
			format->nBlockAlign = (format->wBitsPerSample / 8) * format->nChannels;
			format->nAvgBytesPerSec = format->nBlockAlign * format->nSamplesPerSec;
		}
		if (const auto hr = audioClient->Initialize(AUDCLNT_SHAREMODE_SHARED, streamFlags, period, 0, format._data, nullptr); FAILED(hr))
			return _callbacks.onPlayerError("IAudioClient::Initialize", hr, ::errorToString(hr));
		HandleWrapper event;
		if (event._handle = ::CreateEventW(nullptr, FALSE, FALSE, nullptr); !event._handle)
		{
			const auto errorCode = ::GetLastError();
			return _callbacks.onPlayerError("CreateEventW", errorCode, ::errorToString(errorCode));
		}
		if (const auto hr = audioClient->SetEventHandle(event._handle); FAILED(hr))
			return _callbacks.onPlayerError("IAudioClient::SetEventHandle", hr, ::errorToString(hr));
		UINT32 bufferFrames = 0;
		if (const auto hr = audioClient->GetBufferSize(&bufferFrames); FAILED(hr))
			return _callbacks.onPlayerError("IAudioClient::GetBufferSize", hr, ::errorToString(hr));
		ComPtr<IAudioRenderClient> audioRenderClient;
		if (const auto hr = audioClient->GetService(__uuidof(IAudioRenderClient), reinterpret_cast<void**>(&audioRenderClient)); !audioRenderClient)
			return _callbacks.onPlayerError("IAudioClient::GetService", hr, ::errorToString(hr));
		const UINT32 updateFrames = bufferFrames / kFrameAlignment * kFrameAlignment / 2;
		AudioClientStopper audioClientStopper;
		while (!_stop.load())
		{
			UINT32 lockedFrames = 0;
			for (;;)
			{
				UINT32 paddingFrames = 0;
				if (const auto hr = audioClient->GetCurrentPadding(&paddingFrames); FAILED(hr))
					return _callbacks.onPlayerError("IAudioClient::GetCurrentPadding", hr, ::errorToString(hr));
				lockedFrames = (bufferFrames - paddingFrames) / kFrameAlignment * kFrameAlignment;
				if (lockedFrames >= updateFrames)
					break;
				if (const auto status = ::WaitForSingleObjectEx(event._handle, 2 * paddingFrames * 1000 / _samplingRate, FALSE); status != WAIT_OBJECT_0)
				{
					const auto errorCode = status == WAIT_TIMEOUT ? ERROR_TIMEOUT : ::GetLastError();
					return _callbacks.onPlayerError("WaitForSingleObjectEx", errorCode, ::errorToString(errorCode));
				}
			}
			BYTE* buffer = nullptr;
			if (const auto hr = audioRenderClient->GetBuffer(lockedFrames, &buffer); FAILED(hr))
				return _callbacks.onPlayerError("IAudioRenderClient::GetBuffer", hr, ::errorToString(hr));
			UINT32 readFrames = 0;
			DWORD releaseFlags = 0;
			bool started = false;
			bool stopped = false;
			{
				std::lock_guard lock{ _mutex };
				if (_source)
				{
					if (_sourceChanged)
					{
						_sourceChanged = false;
						started = true;
					}
					readFrames = static_cast<UINT32>(_source->onRead(reinterpret_cast<float*>(buffer), lockedFrames));
					if (readFrames < lockedFrames)
					{
						_source.reset();
						stopped = true;
					}
				}
				else
				{
					readFrames = lockedFrames;
					releaseFlags = AUDCLNT_BUFFERFLAGS_SILENT;
					if (_sourceChanged)
					{
						_sourceChanged = false;
						stopped = true;
					}
				}
			}
			if (const auto hr = audioRenderClient->ReleaseBuffer(readFrames, releaseFlags); FAILED(hr))
				return _callbacks.onPlayerError("IAudioRenderClient::ReleaseBuffer", hr, ::errorToString(hr));
			if (!audioClientStopper._audioClient)
			{
				if (const auto hr = audioClient->Start(); FAILED(hr))
					return _callbacks.onPlayerError("IAudioRenderClient::Start", hr, ::errorToString(hr));
				audioClientStopper._audioClient = audioClient;
			}
			if (started)
				_callbacks.onPlayerStarted();
			if (stopped)
				_callbacks.onPlayerStopped();
		}
	} };
}

PlayerBackend::~PlayerBackend()
{
	_stop.store(true);
	_thread.join();
}

void PlayerBackend::play(const std::shared_ptr<PlayerSource>& source)
{
	auto inOut = source;
	std::lock_guard lock{ _mutex };
	_source.swap(inOut);
	_sourceChanged = true;
}

void PlayerBackend::stop()
{
	decltype(_source) out;
	std::lock_guard lock{ _mutex };
	_source.swap(out);
	_sourceChanged = true;
}
