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

#include "file_output.hpp"

#include "main.hpp"

#include <fstream>

class FileOutput : public Output
{
public:
	FileOutput(const std::filesystem::path& path)
		: _path{ path }
	{
		if (!_stream)
			throw std::runtime_error{ "Failed to open output file" };
	}

	~FileOutput() noexcept
	{
		if (!_committed)
		{
			_stream.close();
			std::error_code ec;
			std::filesystem::remove(_path, ec);
		}
	}

	void commit() override
	{
		if (!_stream.flush())
			throw std::runtime_error{ "Failed to write output data" };
		_committed = true;
	}

	void seek(uint64_t offset) override
	{
		if (!_stream.seekp(offset))
			throw std::runtime_error{ "Failed to write output data" };
	}

	virtual void write(const void* data, size_t size) override
	{
		if (!_stream.write(reinterpret_cast<const char*>(data), size))
			throw std::runtime_error{ "Failed to write output data" };
	}

private:
	const std::filesystem::path _path;
	std::ofstream _stream{ _path, std::ios::binary };
	bool _committed = false;
};

std::unique_ptr<Output> makeFileOutput(const std::filesystem::path& path)
{
	return std::make_unique<FileOutput>(path);
}
