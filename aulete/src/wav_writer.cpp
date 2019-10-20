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

#include "wav_writer.hpp"

#include "main.hpp"

#include <stdexcept>

namespace
{
	constexpr uint32_t makeFourcc(char a, char b, char c, char d) noexcept
	{
		return static_cast<uint8_t>(a) | uint32_t{ static_cast<uint8_t>(b) } << 8
			| uint32_t{ static_cast<uint8_t>(c) } << 16 | uint32_t{ static_cast<uint8_t>(d) } << 24;
	}

	enum class WavFormat : uint16_t
	{
		IeeeFloat = 0x0003,
	};

#pragma pack(push, 1)

	struct WavChunkHeader
	{
		uint32_t _fourcc;
		uint32_t _size;

		constexpr explicit WavChunkHeader(uint32_t fourcc, uint32_t size = 0) noexcept
			: _fourcc{ fourcc }, _size{ size } {}
	};

	struct WavFormatChunk : WavChunkHeader
	{
		WavFormat _format = WavFormat::IeeeFloat;
		uint16_t _channels = 1;
		uint32_t _samplesPerSecond = 48'000;
		uint32_t _bytesPerSecond = _samplesPerSecond * 4;
		uint16_t _blockAlign = 4;
		uint16_t _bitsPerSample = 32;

		constexpr WavFormatChunk() noexcept
			: WavChunkHeader{ makeFourcc('f', 'm', 't', ' '), sizeof(WavFormatChunk) - sizeof(WavChunkHeader) } {}
	};

	struct WavFileHeaders
	{
		WavChunkHeader _riff;
		uint32_t _fourcc = makeFourcc('W', 'A', 'V', 'E');
		WavFormatChunk _fmt;
		WavChunkHeader _data;

		constexpr explicit WavFileHeaders(size_t dataSize) noexcept
			: _riff{ makeFourcc('R', 'I', 'F', 'F'), static_cast<uint32_t>(dataSize + sizeof(WavFileHeaders) - sizeof _riff) }
			, _data{ makeFourcc('d', 'a', 't', 'a'), static_cast<uint32_t>(dataSize) }
		{}
	};

#pragma pack(pop)

	constexpr size_t MaxWavDataSize = std::numeric_limits<uint32_t>::max() - (sizeof(WavFileHeaders) - sizeof(WavChunkHeader));
}

class WavWriter : public Writer
{
public:
	WavWriter(std::unique_ptr<Output>&& output)
		: _output{ std::move(output) }
	{
		WavFileHeaders headers{ 0 };
		_output->write(&headers, sizeof headers);
	}

	void commit() override
	{
		_output->seek(0);
		WavFileHeaders headers{ _dataWritten };
		_output->write(&headers, sizeof headers);
		_output->commit();
	}

	void write(const void* data, size_t size) override
	{
		if (size > MaxWavDataSize - _dataWritten)
			throw std::runtime_error{ "Failed to write output file data" };
		_output->write(data, size);
		_dataWritten += size;
	}

private:
	const std::unique_ptr<Output> _output;
	size_t _dataWritten = 0;
};

std::unique_ptr<Writer> makeWavWriter(std::unique_ptr<Output>&& output)
{
	return std::make_unique<WavWriter>(std::move(output));
}
