// This file is part of the Aulos toolkit.
// Copyright (C) Sergei Blagodarin.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <cassert>
#include <cstdlib>
#include <type_traits>

namespace aulos
{
	template <typename T>
	class StaticVector
	{
	public:
		constexpr StaticVector() noexcept = default;
		StaticVector(const StaticVector&) = delete;
		StaticVector& operator=(const StaticVector&) = delete;

		~StaticVector() noexcept
		{
			clear();
			std::free(_data);
		}

		[[nodiscard]] constexpr T* begin() noexcept { return _data; }
		[[nodiscard]] constexpr const T* begin() const noexcept { return _data; }
		[[nodiscard]] constexpr const T* cbegin() const noexcept { return _data; }
		[[nodiscard]] constexpr const T* cend() const noexcept { return _data + _size; }
		[[nodiscard]] constexpr bool empty() const noexcept { return !_size; }
		[[nodiscard]] constexpr T* end() noexcept { return _data + _size; }
		[[nodiscard]] constexpr const T* end() const noexcept { return _data + _size; }
		[[nodiscard]] constexpr size_t size() const noexcept { return _size; }

		[[nodiscard]] constexpr T& back() noexcept
		{
			assert(_size > 0);
			return _data[_size - 1];
		}

		[[nodiscard]] constexpr const T& back() const noexcept
		{
			assert(_size > 0);
			return _data[_size - 1];
		}

		void clear() noexcept
		{
			if constexpr (!std::is_trivially_destructible_v<T>)
				for (size_t i = 0; i < _size; ++i)
					_data[i].~T();
			_size = 0;
		}

		template <typename... Args>
		T& emplace_back(Args&&... args)
		{
			assert(_size < _capacity);
			T* value = new (_data + _size) T{ std::forward<Args>(args)... };
			++_size;
			return *value;
		}

		void pop_back() noexcept
		{
			assert(_size > 0);
			--_size;
			if constexpr (!std::is_trivially_destructible_v<T>)
				_data[_size].~T();
		}

		void reserve(size_t capacity)
		{
			assert(!_data);
			_data = static_cast<T*>(std::malloc(capacity * sizeof(T)));
			if (!_data)
				throw std::bad_alloc{};
			_capacity = capacity;
		}

		[[nodiscard]] T& operator[](size_t index) noexcept
		{
			assert(index < _size);
			return _data[index];
		}

		[[nodiscard]] const T& operator[](size_t index) const noexcept
		{
			assert(index < _size);
			return _data[index];
		}

	private:
		T* _data = nullptr;
		size_t _size = 0;
		size_t _capacity = 0;
	};
}
