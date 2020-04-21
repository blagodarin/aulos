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
#include <cassert>
#include <chrono>
#include <filesystem>
#include <functional>
#include <fstream>
#include <iostream>
#include <limits>
#include <string>

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

	std::string print(const std::chrono::nanoseconds& duration)
	{
		struct Bound
		{
			const char* _units;
			std::chrono::nanoseconds::rep _scale;
			std::chrono::nanoseconds::rep _maximum;
		};

		static const std::array<Bound, 10> bounds{
			Bound{ "ns", 1, 999 },
			Bound{ "us", 100, 9'999 },
			Bound{ "us", 10, 99'999 },
			Bound{ "us", 1, 999'999 },
			Bound{ "ms", 100, 9'999'999 },
			Bound{ "ms", 10, 99'999'999 },
			Bound{ "ms", 1, 999'999'999 },
			Bound{ "s", 100, 9'999'999'999 },
			Bound{ "s", 10, 99'999'999'999 },
			Bound{ "s", 1, std::numeric_limits<std::chrono::nanoseconds::rep>::max() },
		};

		const auto nanoseconds = duration.count();
		const auto i = std::find_if_not(bounds.begin(), bounds.end(), [nanoseconds](const Bound& bound) { return nanoseconds > bound._maximum; });
		assert(i != bounds.end());
		const auto scale = (i->_maximum + 1) / 1'000;
		const auto value = (nanoseconds + scale - 1) / scale;
		const auto whole = value / i->_scale;
		const auto fraction = value % i->_scale;
		return (fraction ? std::to_string(whole) + '.' + std::to_string(fraction) : std::to_string(whole)) + i->_units;
	}

	struct Measurement
	{
		using Duration = std::chrono::high_resolution_clock::duration;

		Duration::rep _iterations = 0;
		Duration _totalDuration{ 0 };
		Duration _minDuration = Duration::max();
		Duration _maxDuration{ 0 };

		auto average() const noexcept { return Duration{ (_totalDuration.count() + _iterations - 1) / _iterations }; }
	};

	template <Measurement::Duration::rep maxIterations = std::numeric_limits<Measurement::Duration::rep>::max(), typename Payload, typename Cleanup>
	auto measure(Payload&& payload, Cleanup&& cleanup, const std::chrono::seconds& minDuration = std::chrono::seconds{ 1 })
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
			if (measurement._iterations >= maxIterations || measurement._totalDuration >= minDuration)
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
	const auto parsing = ::measure<10'000>(
		[&composition, source = data.get()] { composition = aulos::Composition::create(source); }, // Clang 9 is unable to capture 'data' by reference.
		[&composition] { composition.reset(); });

	std::unique_ptr<aulos::Renderer> renderer;
	const auto preparation = ::measure<10'000>(
		[&renderer, &composition, samplingRate] { renderer = aulos::Renderer::create(*composition, samplingRate, 2); },
		[&renderer] { renderer.reset(); });

	const auto rendering = ::measure(
		[&renderer] { while (renderer->render(buffer.data(), buffer.size()) > 0) ; },
		[&renderer] { renderer->restart(); },
		std::chrono::seconds{ 5 });

	const auto compositionDuration = renderer->totalSamples() * double{ Measurement::Duration::period::den } / samplingRate;

	std::cout << "ParseTime: " << ::print(parsing.average()) << " [N=" << parsing._iterations << ", min=" << ::print(parsing._minDuration) << ", max=" << ::print(parsing._maxDuration) << "]\n";
	std::cout << "PrepareTime: " << ::print(preparation.average()) << " [N=" << preparation._iterations << ", min=" << ::print(preparation._minDuration) << ", max=" << ::print(preparation._maxDuration) << "]\n";
	std::cout << "RenderTime: " << ::print(rendering.average()) << " [N=" << rendering._iterations << ", min=" << ::print(rendering._minDuration) << ", max=" << ::print(rendering._maxDuration) << "]\n";
	std::cout << "RenderSpeed: " << compositionDuration / rendering.average().count() << "x\n";
	return 0;
}
