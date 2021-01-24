// This file is part of the Aulos toolkit.
// Copyright (C) Sergei Blagodarin.
// SPDX-License-Identifier: Apache-2.0

#include "backend.hpp"

#include <primal/buffer.hpp>
#include <primal/c_ptr.hpp>

#define WIN32_LEAN_AND_MEAN
#pragma warning(push)
#pragma warning(disable : 4365) // signed/unsigned mismatch
#pragma warning(disable : 4668) // '_WIN64' is not defined as a preprocessor macro, replacing with '0' for '#if/#elif'
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
	using ComPtr = _com_ptr_t<_com_IIID<T, &__uuidof(T)>>;

	struct ComUninitializer
	{
		~ComUninitializer() noexcept { ::CoUninitialize(); }
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
}

namespace aulosplay
{
	void runBackend(BackendCallbacks& callbacks, unsigned samplingRate)
	{
		const auto error = [&callbacks](const char* function, HRESULT code) {
			std::string description;
			{
				struct LocalBuffer
				{
					char* _data = nullptr;
					~LocalBuffer() noexcept { ::LocalFree(_data); }
				};
				LocalBuffer buffer;
				::FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
					nullptr, static_cast<DWORD>(code), MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), reinterpret_cast<char*>(&buffer._data), 0, nullptr);
				if (buffer._data)
				{
					auto length = std::strlen(buffer._data);
					if (length > 0 && buffer._data[length - 1] == '\n')
					{
						--length;
						if (length > 0 && buffer._data[length - 1] == '\r')
							--length;
					}
					description.assign(buffer._data, length);
				}
			}
			callbacks.onBackendError(function, static_cast<int>(code), description);
		};

		if (const auto hr = ::CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED); FAILED(hr))
			return error("CoInitializeEx", hr);
		ComUninitializer comUninitializer;
		ComPtr<IMMDeviceEnumerator> deviceEnumerator;
		if (const auto hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), reinterpret_cast<void**>(&deviceEnumerator)); !deviceEnumerator)
			return error("CoCreateInstance", hr);
		ComPtr<IMMDevice> device;
		if (const auto hr = deviceEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &device); !device)
			return hr == E_NOTFOUND ? callbacks.onBackendError(PlaybackError::NoDevice) : error("IMMDeviceEnumerator::GetDefaultAudioEndpoint", hr);
		ComPtr<IAudioClient> audioClient;
		if (const auto hr = device->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr, reinterpret_cast<void**>(&audioClient)); !audioClient)
			return error("IMMDevice::Activate", hr);
		REFERENCE_TIME period = 0;
		if (const auto hr = audioClient->GetDevicePeriod(nullptr, &period); FAILED(hr))
			return error("IAudioClient::GetDevicePeriod", hr);
		primal::CPtr<WAVEFORMATEX, ::CoTaskMemFree> format;
		if (const auto hr = audioClient->GetMixFormat(format.out()); !format)
			return error("IAudioClient::GetMixFormat", hr);
		if (format->wFormatTag == WAVE_FORMAT_EXTENSIBLE)
		{
			const auto extensible = reinterpret_cast<WAVEFORMATEXTENSIBLE*>(format.get());
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
		if (format->nChannels != kBackendChannels)
		{
			format->nChannels = kBackendChannels;
			format->nBlockAlign = static_cast<WORD>((format->wBitsPerSample / 8) * format->nChannels);
			format->nAvgBytesPerSec = format->nBlockAlign * format->nSamplesPerSec;
		}
		if (const auto hr = audioClient->Initialize(AUDCLNT_SHAREMODE_SHARED, streamFlags, period, 0, format, nullptr); FAILED(hr))
			return error("IAudioClient::Initialize", hr);
		primal::CPtr<std::remove_pointer_t<HANDLE>, ::CloseHandle> event;
		if (*event.out() = ::CreateEventW(nullptr, FALSE, FALSE, nullptr); !event)
			return error("CreateEventW", static_cast<HRESULT>(::GetLastError()));
		if (const auto hr = audioClient->SetEventHandle(event); FAILED(hr))
			return error("IAudioClient::SetEventHandle", hr);
		UINT32 bufferFrames = 0;
		if (const auto hr = audioClient->GetBufferSize(&bufferFrames); FAILED(hr))
			return error("IAudioClient::GetBufferSize", hr);
		ComPtr<IAudioRenderClient> audioRenderClient;
		if (const auto hr = audioClient->GetService(__uuidof(IAudioRenderClient), reinterpret_cast<void**>(&audioRenderClient)); !audioRenderClient)
			return error("IAudioClient::GetService", hr);
		const UINT32 updateFrames = bufferFrames / kBackendFrameAlignment * kBackendFrameAlignment / 2;
		primal::Buffer<float, primal::AlignedAllocator<kSimdAlignment>> monoBuffer{ bufferFrames };
		AudioClientStopper audioClientStopper;
		while (callbacks.onBackendIdle())
		{
			UINT32 lockedFrames = 0;
			for (;;)
			{
				UINT32 paddingFrames = 0;
				if (const auto hr = audioClient->GetCurrentPadding(&paddingFrames); FAILED(hr))
					return error("IAudioClient::GetCurrentPadding", hr);
				lockedFrames = (bufferFrames - paddingFrames) / kBackendFrameAlignment * kBackendFrameAlignment;
				if (lockedFrames >= updateFrames)
					break;
				if (const auto status = ::WaitForSingleObjectEx(event, 2 * paddingFrames * 1000 / samplingRate, FALSE); status != WAIT_OBJECT_0)
					return error("WaitForSingleObjectEx", static_cast<HRESULT>(status == WAIT_TIMEOUT ? ERROR_TIMEOUT : ::GetLastError()));
			}
			BYTE* buffer = nullptr;
			if (const auto hr = audioRenderClient->GetBuffer(lockedFrames, &buffer); FAILED(hr))
				return error("IAudioRenderClient::GetBuffer", hr);
			auto writtenFrames = static_cast<UINT32>(callbacks.onBackendRead(reinterpret_cast<float*>(buffer), lockedFrames, monoBuffer.data()));
			DWORD releaseFlags = 0;
			if (!writtenFrames)
			{
				writtenFrames = lockedFrames;
				releaseFlags = AUDCLNT_BUFFERFLAGS_SILENT;
			}
			if (const auto hr = audioRenderClient->ReleaseBuffer(writtenFrames, releaseFlags); FAILED(hr))
				return error("IAudioRenderClient::ReleaseBuffer", hr);
			if (!audioClientStopper._audioClient)
			{
				if (const auto hr = audioClient->Start(); FAILED(hr))
					return error("IAudioRenderClient::Start", hr);
				audioClientStopper._audioClient = audioClient;
			}
		}
	}
}
