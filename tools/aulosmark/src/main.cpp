//
// This file is part of the Aulos toolkit.
// Copyright (C) 2020 Sergei Blagodarin.
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

#include <aulos/playback.hpp>

#include <array>
#include <chrono>
#include <filesystem>
#include <functional>
#include <fstream>
#include <iostream>

namespace
{
	std::pair<std::unique_ptr<char[]>, size_t> load(const std::filesystem::path& path)
	{
		std::ifstream stream{ path, std::ios::binary };
		if (!stream)
		{
			std::cerr << "Failed to open input file\n";
			return {};
		}
		stream.seekg(0, std::ios::end);
		const auto size = static_cast<size_t>(stream.tellg());
		auto result = std::make_unique<char[]>(size + 1);
		stream.seekg(0, std::ios::beg);
		stream.read(result.get(), static_cast<std::streamsize>(size));
		result[size] = '\0';
		return { std::move(result), size };
	}

	struct Measurement
	{
		using Duration = std::chrono::high_resolution_clock::duration;

		Duration::rep _iterations = 0;
		Duration _totalDuration{ 0 };
		Duration _minDuration = Duration::max();
		Duration _maxDuration{ 0 };

		auto average() const noexcept { return (std::chrono::duration_cast<std::chrono::microseconds>(_totalDuration).count() + _iterations - 1) / _iterations; }
		auto maximum() const noexcept { return std::chrono::duration_cast<std::chrono::microseconds>(_maxDuration).count(); }
		auto minimum() const noexcept { return std::chrono::duration_cast<std::chrono::microseconds>(_minDuration).count(); }
	};

	template <typename Payload, typename Cleanup>
	auto measure(Payload&& payload, Cleanup&& cleanup)
	{
		Measurement measurement;
		for (;;)
		{
			const auto startTime = std::chrono::high_resolution_clock::now();
			payload();
			const auto duration = std::chrono::high_resolution_clock::now() - startTime;
			++measurement._iterations;
			measurement._totalDuration += duration;
			if (measurement._minDuration > duration)
				measurement._minDuration = duration;
			if (measurement._maxDuration < duration)
				measurement._maxDuration = duration;
			if (measurement._totalDuration >= std::chrono::seconds{ 1 })
				break;
			cleanup();
		}
		return measurement;
	}

	std::array<std::byte, 65'536> buffer;
}

int main(int argc, char** argv)
{
	std::filesystem::path path;
	unsigned samplingRate = 48'000;
	if (int i = 1; i < argc)
		path = argv[i];
	else
	{
		std::cerr << "No input file specified\n";
		return 1;
	}

	const auto [data, size] = ::load(path);
	if (!data)
		return 1;

	std::unique_ptr<aulos::Composition> composition;
	const auto parsing = ::measure(
		[&composition, source = data.get()] { composition = aulos::Composition::create(source); }, // Clang 9 is unable to capture 'data' by reference.
		[&composition] { composition.reset(); });

	std::unique_ptr<aulos::Renderer> renderer;
	const auto preparation = ::measure(
		[&renderer, &composition, samplingRate] { renderer = aulos::Renderer::create(*composition, samplingRate, 2); },
		[&renderer] { renderer.reset(); });

	const auto rendering = ::measure(
		[&renderer] { while (renderer->render(buffer.data(), buffer.size()) > 0) ; },
		[&renderer] { renderer->restart(); });

	const auto compositionDuration = renderer->totalSamples() * double{ std::chrono::microseconds::period::den } / samplingRate;

	std::cout << "ParseTime: " << parsing.average() << "us [N=" << parsing._iterations << ", min=" << parsing.minimum() << "us, max=" << parsing.maximum() << "us]\n";
	std::cout << "PrepareTime: " << preparation.average() << "us [N=" << preparation._iterations << ", min=" << preparation.minimum() << "us, max=" << preparation.maximum() << "us]\n";
	std::cout << "RenderTime: " << rendering.average() << "us [N=" << rendering._iterations << ", min=" << rendering.minimum() << "us, max=" << rendering.maximum() << "us]\n";
	std::cout << "RenderSpeed: " << compositionDuration / rendering.average() << "x\n";
	return 0;
}
