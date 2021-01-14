// This file is part of the Aulos toolkit.
// Copyright (C) Sergei Blagodarin.
// SPDX-License-Identifier: Apache-2.0

#include <type_traits>

namespace aulosplay
{
	template <typename T, auto deleter>
	class CPtr
	{
	public:
		using Value = std::remove_all_extents_t<std::remove_reference_t<T>>;

		constexpr CPtr() noexcept = default;
		CPtr(const CPtr&) = delete;
		CPtr& operator=(const CPtr&) = delete;

		constexpr explicit CPtr(Value* pointer) noexcept
			: _pointer{ pointer } {}

		~CPtr() noexcept
		{
			if (_pointer)
				deleter(_pointer);
		}

		constexpr operator Value*() const noexcept { return _pointer; }
		constexpr Value* operator->() const noexcept { return _pointer; }

		constexpr Value* get() const noexcept { return _pointer; }
		constexpr Value** out() noexcept { return &_pointer; }

	private:
		Value* _pointer = nullptr;
	};
}
