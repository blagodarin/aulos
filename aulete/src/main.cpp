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

#include "main.hpp"

#include "file_output.hpp"
#include "wav_writer.hpp"

#include <array>
#include <iostream>

#include <aulos.hpp>

int main(int argc, char** argv)
{
	if (argc != 2)
	{
		std::cerr << "Usage: " << std::filesystem::path{ argv[0] } << " OUTFILE\n";
		return 1;
	}
	try
	{
		const auto writer = makeWavWriter(makeFileOutput(argv[1]));
		aulos::Renderer renderer{ "", 0 };
		std::array<uint8_t, 1024> buffer;
		while (const auto bytesRendered = renderer.render(buffer.data(), buffer.size()))
			writer->write(buffer.data(), bytesRendered);
		writer->commit();
	}
	catch (const std::runtime_error& e)
	{
		std::cerr << e.what() << '\n';
		return 1;
	}
	return 0;
}
