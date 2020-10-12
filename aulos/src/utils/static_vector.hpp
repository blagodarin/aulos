// This file is part of the Aulos toolkit.
// Copyright (C) Sergei Blagodarin.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <cassert>
#include <cstdlib>
#include <memory>
#include <type_traits>
#include <utility>

namespace aulos
{
	// std::vector-like container with preallocated storage (like std::array).
	template <typename T, size_t kCapacity>
	class StaticVector
	{
	public:
		constexpr StaticVector() noexcept = default;

		StaticVector(const StaticVector&) = delete;

		~StaticVector() noexcept
		{
			clear();
		}

		StaticVector& operator=(const StaticVector&) = delete;

		[[nodiscard]] constexpr T* begin() noexcept { return reinterpret_cast<T*>(_data); }
		[[nodiscard]] constexpr const T* begin() const noexcept { return reinterpret_cast<const T*>(_data); }
		[[nodiscard]] constexpr const T* cbegin() const noexcept { return reinterpret_cast<const T*>(_data); }
		[[nodiscard]] constexpr const T* cend() const noexcept { return reinterpret_cast<const T*>(_data + _size); }
		[[nodiscard]] constexpr bool empty() const noexcept { return !_size; }
		[[nodiscard]] constexpr T* end() noexcept { return reinterpret_cast<T*>(_data + _size); }
		[[nodiscard]] constexpr const T* end() const noexcept { return reinterpret_cast<const T*>(_data + _size); }
		[[nodiscard]] constexpr size_t size() const noexcept { return _size; }

		[[nodiscard]] constexpr T& back() noexcept
		{
			assert(_size > 0);
			return reinterpret_cast<T*>(_data)[_size - 1];
		}

		[[nodiscard]] constexpr const T& back() const noexcept
		{
			assert(_size > 0);
			return reinterpret_cast<const T*>(_data)[_size - 1];
		}

		void clear() noexcept
		{
			std::destroy_n(reinterpret_cast<T*>(_data), _size);
			_size = 0;
		}

		template <typename... Args>
		T& emplace_back(Args&&... args)
		{
			assert(_size < kCapacity);
			T* value = new (_data + _size) T{ std::forward<Args>(args)... };
			++_size;
			return *value;
		}

		void pop_back() noexcept
		{
			assert(_size > 0);
			--_size;
			std::destroy_at(reinterpret_cast<T*>(_data) + _size);
		}

		[[nodiscard]] T& operator[](size_t index) noexcept
		{
			assert(index < _size);
			return static_cast<T*>(_data)[index];
		}

		[[nodiscard]] const T& operator[](size_t index) const noexcept
		{
			assert(index < _size);
			return static_cast<const T*>(_data)[index];
		}

	private:
		size_t _size = 0;
		std::aligned_storage_t<sizeof(T), alignof(T)> _data[kCapacity];
	};
}
