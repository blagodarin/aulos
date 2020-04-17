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
		stream.read(result.get(), size);
		result[size] = '\0';
		return { std::move(result), size };
	}

	template <typename Payload, typename Cleanup>
	auto measure(Payload&& payload, Cleanup&& cleanup)
	{
		std::chrono::high_resolution_clock::duration duration{ 0 };
		std::chrono::microseconds::rep iterations = 0;
		for (;;)
		{
			const auto startTime = std::chrono::high_resolution_clock::now();
			payload();
			duration += std::chrono::high_resolution_clock::now() - startTime;
			++iterations;
			if (duration >= std::chrono::seconds{ 1 })
				break;
			cleanup();
		}
		return std::pair{ (std::chrono::duration_cast<std::chrono::microseconds>(duration).count() + iterations - 1) / iterations, iterations };
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
	const auto [parseTime, parseIterations] = ::measure(
		[&composition, &data] { composition = aulos::Composition::create(data.get()); },
		[&composition] { composition.reset(); });

	std::unique_ptr<aulos::Renderer> renderer;
	const auto [prepareTime, prepareIterations] = ::measure(
		[&renderer, &composition, samplingRate] { renderer = aulos::Renderer::create(*composition, samplingRate, 2); },
		[&renderer] { renderer.reset(); });

	const auto [renderTime, renderIterations] = ::measure(
		[&renderer] { while (renderer->render(buffer.data(), buffer.size()) > 0) ; },
		[&renderer] { renderer->restart(); });

	const auto compositionDuration = renderer->totalSamples() * double{ std::chrono::microseconds::period::den } / samplingRate;

	std::cout << "ParseTime: " << parseTime << "us [N=" << parseIterations << "]\n";
	std::cout << "PrepareTime: " << prepareTime << "us [N=" << prepareIterations << "]\n";
	std::cout << "RenderTime: " << renderTime << "us [N=" << renderIterations << "]\n";
	std::cout << "RenderSpeed: " << compositionDuration / renderTime << "x\n";
	return 0;
}
