// This file is part of the Aulos toolkit.
// Copyright (C) Sergei Blagodarin.
// SPDX-License-Identifier: Apache-2.0

#include "backend.hpp"

#include <aulosplay/player.hpp>

#define WIN32_LEAN_AND_MEAN
#pragma warning(push)
#pragma warning(disable : 4365) // signed/unsigned mismatch
#pragma warning(disable : 5039) // pointer or reference to potentially throwing function passed to 'extern "C"' function under -EHc. Undefined behavior may occur if this function throws an exception.
#pragma warning(disable : 5204) // class has virtual functions, but its trivial destructor is not virtual; instances of objects derived from this class may not be destructed correctly
#include <audioclient.h>
#include <comdef.h>
#include <mmdeviceapi.h>
#include <winnt.h>
#pragma warning(pop)

namespace
{
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

	std::string errorToString(DWORD error)
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

namespace aulosplay
{
	void runBackend(PlayerCallbacks& callbacks, unsigned samplingRate, std::atomic<size_t>& offset, const std::atomic<bool>& stopFlag, const std::function<size_t(float*, size_t)>& mixFunction)
	{
		const auto reportError = [&callbacks](const char* function, auto error) {
			callbacks.onPlaybackError(function, static_cast<DWORD>(error), ::errorToString(static_cast<DWORD>(error)));
		};

		if (const auto hr = ::CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED); FAILED(hr))
			return reportError("CoInitializeEx", hr);
		ComUninitializer comUninitializer;
		ComPtr<IMMDeviceEnumerator> deviceEnumerator;
		if (const auto hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), reinterpret_cast<void**>(&deviceEnumerator)); !deviceEnumerator)
			return reportError("CoCreateInstance", hr);
		ComPtr<IMMDevice> device;
		if (const auto hr = deviceEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &device); !device)
			return reportError("IMMDeviceEnumerator::GetDefaultAudioEndpoint", hr);
		ComPtr<IAudioClient> audioClient;
		if (const auto hr = device->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr, reinterpret_cast<void**>(&audioClient)); !audioClient)
			return reportError("IMMDevice::Activate", hr);
		REFERENCE_TIME period = 0;
		if (const auto hr = audioClient->GetDevicePeriod(nullptr, &period); FAILED(hr))
			return reportError("IAudioClient::GetDevicePeriod", hr);
		ComBuffer<WAVEFORMATEX> format;
		if (const auto hr = audioClient->GetMixFormat(&format._data); !format._data)
			return reportError("IAudioClient::GetMixFormat", hr);
		if (format->wFormatTag == WAVE_FORMAT_EXTENSIBLE)
		{
			const auto extensible = reinterpret_cast<WAVEFORMATEXTENSIBLE*>(format._data);
			if (!::IsEqualGUID(extensible->SubFormat, KSDATAFORMAT_SUBTYPE_IEEE_FLOAT) || extensible->Format.wBitsPerSample != 32)
			{
				extensible->Format.wBitsPerSample = 32;
				extensible->Format.nBlockAlign = static_cast<WORD>((format->wBitsPerSample / 8) * format->nChannels);
				extensible->Format.nAvgBytesPerSec = format->nBlockAlign * format->nSamplesPerSec;
				extensible->Samples.wValidBitsPerSample = 32;
				extensible->SubFormat = KSDATAFORMAT_SUBTYPE_IEEE_FLOAT;
			}
		}
		else if (format->wFormatTag != WAVE_FORMAT_IEEE_FLOAT || format->wBitsPerSample != 32)
		{
			format->wFormatTag = WAVE_FORMAT_IEEE_FLOAT;
			format->wBitsPerSample = 32;
			format->nBlockAlign = static_cast<WORD>((format->wBitsPerSample / 8) * format->nChannels);
			format->nAvgBytesPerSec = format->nBlockAlign * format->nSamplesPerSec;
		}
		DWORD streamFlags = AUDCLNT_STREAMFLAGS_EVENTCALLBACK;
		if (format->nSamplesPerSec != samplingRate)
		{
			streamFlags |= AUDCLNT_STREAMFLAGS_RATEADJUST;
			format->nSamplesPerSec = samplingRate;
			format->nAvgBytesPerSec = format->nBlockAlign * format->nSamplesPerSec;
		}
		if (format->nChannels != kChannels)
		{
			format->nChannels = kChannels;
			format->nBlockAlign = static_cast<WORD>((format->wBitsPerSample / 8) * format->nChannels);
			format->nAvgBytesPerSec = format->nBlockAlign * format->nSamplesPerSec;
		}
		if (const auto hr = audioClient->Initialize(AUDCLNT_SHAREMODE_SHARED, streamFlags, period, 0, format._data, nullptr); FAILED(hr))
			return reportError("IAudioClient::Initialize", hr);
		HandleWrapper event;
		if (event._handle = ::CreateEventW(nullptr, FALSE, FALSE, nullptr); !event._handle)
			return reportError("CreateEventW", ::GetLastError());
		if (const auto hr = audioClient->SetEventHandle(event._handle); FAILED(hr))
			return reportError("IAudioClient::SetEventHandle", hr);
		UINT32 bufferFrames = 0;
		if (const auto hr = audioClient->GetBufferSize(&bufferFrames); FAILED(hr))
			return reportError("IAudioClient::GetBufferSize", hr);
		ComPtr<IAudioRenderClient> audioRenderClient;
		if (const auto hr = audioClient->GetService(__uuidof(IAudioRenderClient), reinterpret_cast<void**>(&audioRenderClient)); !audioRenderClient)
			return reportError("IAudioClient::GetService", hr);
		const UINT32 updateFrames = bufferFrames / kFrameAlignment * kFrameAlignment / 2;
		AudioClientStopper audioClientStopper;
		for (bool wasSilent = true; !stopFlag.load();)
		{
			UINT32 lockedFrames = 0;
			for (;;)
			{
				UINT32 paddingFrames = 0;
				if (const auto hr = audioClient->GetCurrentPadding(&paddingFrames); FAILED(hr))
					return reportError("IAudioClient::GetCurrentPadding", hr);
				lockedFrames = (bufferFrames - paddingFrames) / kFrameAlignment * kFrameAlignment;
				if (lockedFrames >= updateFrames)
					break;
				if (const auto status = ::WaitForSingleObjectEx(event._handle, 2 * paddingFrames * 1000 / samplingRate, FALSE); status != WAIT_OBJECT_0)
				{
					const auto errorCode = status == WAIT_TIMEOUT ? ERROR_TIMEOUT : ::GetLastError();
					return callbacks.onPlaybackError("WaitForSingleObjectEx", errorCode, ::errorToString(errorCode));
				}
			}
			BYTE* buffer = nullptr;
			if (const auto hr = audioRenderClient->GetBuffer(lockedFrames, &buffer); FAILED(hr))
				return reportError("IAudioRenderClient::GetBuffer", hr);
			auto writtenFrames = static_cast<UINT32>(mixFunction(reinterpret_cast<float*>(buffer), lockedFrames));
			DWORD releaseFlags = 0;
			const auto isSilent = writtenFrames == 0;
			if (isSilent)
			{
				writtenFrames = lockedFrames;
				releaseFlags = AUDCLNT_BUFFERFLAGS_SILENT;
			}
			if (const auto hr = audioRenderClient->ReleaseBuffer(writtenFrames, releaseFlags); FAILED(hr))
				return reportError("IAudioRenderClient::ReleaseBuffer", hr);
			if (!audioClientStopper._audioClient)
			{
				if (const auto hr = audioClient->Start(); FAILED(hr))
					return reportError("IAudioRenderClient::Start", hr);
				audioClientStopper._audioClient = audioClient;
			}
			if (isSilent != wasSilent)
			{
				wasSilent = isSilent;
				if (!isSilent)
				{
					offset.store(0);
					callbacks.onPlaybackStarted();
				}
				else
					callbacks.onPlaybackStopped();
			}
			else if (!isSilent)
				offset.fetch_add(lockedFrames);
		}
	}
}
