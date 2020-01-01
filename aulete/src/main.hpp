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

#pragma once

#include <cstdint>

class Output
{
public:
	virtual ~Output() noexcept = default;

	virtual void commit() = 0;
	virtual void seek(uint64_t) = 0;
	virtual void write(const void* data, size_t size) = 0;
};

class Writer
{
public:
	virtual ~Writer() noexcept = default;

	virtual void* buffer() = 0;
	virtual size_t bufferSize() const noexcept = 0;
	virtual void commit() = 0;
	virtual void write(size_t size) = 0;
};
