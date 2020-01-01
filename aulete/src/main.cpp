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

#include "main.hpp"

#include "file_output.hpp"
#include "playback_writer.hpp"
#include "wav_writer.hpp"

#include <array>
#include <fstream>
#include <functional>
#include <iostream>
#include <optional>

#include <aulos.hpp>

namespace
{
	std::string loadFile(const std::filesystem::path& path)
	{
		std::ifstream stream{ path, std::ios::binary };
		if (!stream)
			throw std::runtime_error{ "Failed to open input file" };
		stream.seekg(0, std::ios::end);
		std::string result(stream.tellg(), '\0');
		stream.seekg(0, std::ios::beg);
		stream.read(result.data(), result.size());
		return result;
	}

	struct Options
	{
		std::filesystem::path _input;
		std::filesystem::path _output;
		unsigned _samplingRate = 48'000;
	};

	bool parseArguments(int argc, char** argv, Options& options)
	{
		for (int i = 1; i < argc;)
		{
			const std::string_view argument{ argv[i++] };
			if (argument == "-o")
			{
				if (i == argc)
					return false;
				options._output = argv[i++];
			}
			else if (argument == "-s")
			{
				if (i == argc)
					return false;
				options._samplingRate = std::atoi(argv[i++]);
			}
			else if (argument[0] != '-')
				options._input = argument;
			else
				return false;
		}
		return !options._input.empty();
	}
}

int main(int argc, char** argv)
{
	try
	{
		Options options;
		if (!parseArguments(argc, argv, options))
			throw std::runtime_error{ "Usage:\n\t" + std::filesystem::path{ argv[0] }.filename().string() + " INFILE [-o OUTFILE] [-s SAMPLERATE]" };
		const auto writer = options._output.empty()
			? makePlaybackWriter(options._samplingRate)
			: makeWavWriter(options._samplingRate, makeFileOutput(options._output));
		const auto composition = aulos::Composition::create(loadFile(options._input).c_str());
		const auto renderer = aulos::Renderer::create(*composition, options._samplingRate);
		for (;;)
			if (const auto bytesRendered = renderer->render(writer->buffer(), writer->bufferSize()); bytesRendered > 0)
				writer->write(bytesRendered);
			else
				break;
		writer->commit();
	}
	catch (const std::runtime_error& e)
	{
		std::cerr << e.what() << '\n';
		return 1;
	}
	return 0;
}
