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

#include <array>
#include <iostream>
#include <filesystem>
#include <fstream>

#include <aulos.hpp>

#include "wav_writer.hpp"

class FileOutput
{
public:
	FileOutput(const std::filesystem::path& path)
		: _path{ path } {}

	~FileOutput() noexcept
	{
		if (!_committed)
		{
			_stream.close();
			std::error_code ec;
			std::filesystem::remove(_path, ec);
		}
	}

	void commit() noexcept { _committed = true; }
	const std::filesystem::path& path() const noexcept { return _path; }
	std::ostream& stream() noexcept { return _stream; }

private:
	const std::filesystem::path _path;
	std::ofstream _stream{ _path, std::ios::binary };
	bool _committed = false;
};

int main(int argc, char** argv)
{
	if (argc != 2)
	{
		std::cerr << "Usage: " << std::filesystem::path{ argv[0] } << " OUTFILE\n";
		return 1;
	}

	FileOutput output{ argv[1] };
	if (!output.stream())
	{
		std::cerr << "Failed to open file: " << output.path() << "\n";
		return 2;
	}

	WavWriter writer{ output.stream() };
	if (!writer)
	{
		std::cerr << "Failed to write file headers: " << output.path() << "\n";
		return 3;
	}

	aulos::Renderer renderer{ "", 0 };
	std::array<uint8_t, 1024> buffer;
	for (;;)
	{
		const auto bytesRendered = renderer.render(buffer.data(), buffer.size());
		if (!bytesRendered)
			break;
		if (!writer.write(buffer.data(), bytesRendered))
		{
			std::cerr << "Failed to write file data: " << output.path() << "\n";
			return 4;
		}
	}

	if (!writer.commit())
	{
		std::cerr << "Failed to finalize file: " << output.path() << "\n";
		return 5;
	}

	output.commit();
	return 0;
}
