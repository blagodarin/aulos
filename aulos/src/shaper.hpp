// This file is part of the Aulos toolkit.
// Copyright (C) Sergei Blagodarin.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <cassert>
#include <cmath>
#include <numbers>
#include <type_traits>

namespace aulos
{
	// Shaper is a stateful object that advances from (0, firstY) to (deltaX, firstY + deltaY) according to the shape function Y(X)
	// which stays in [firstY, firstY + deltaY] (or [firstY + deltaY, firstY] if deltaY is negative) for any X in [0, deltaX].
	// Shapers start at offsetX which must be in [0, deltaX).

	struct ShaperData
	{
		float _firstY = 0;
		float _deltaY = 0;
		float _deltaX = 1;
		float _shape = 0;
		float _offsetX = 0;
	};

	// C1 = deltaY / deltaX
	// Y(X) = firstY + C1 * X
	// Y(X + 1) = Y(X) + C1
	class LinearShaper
	{
	public:
		explicit constexpr LinearShaper(const ShaperData& data) noexcept
			: _c1{ data._deltaY / data._deltaX }
			, _nextY{ data._firstY + _c1 * data._offsetX }
		{
		}

		constexpr auto advance() noexcept
		{
			const auto nextY = _nextY;
			_nextY += _c1;
			return static_cast<float>(nextY);
		}

		template <typename Float, typename = std::enable_if_t<std::is_floating_point_v<Float>>>
		static constexpr auto value(Float firstY, Float deltaY, Float deltaX, Float, Float offsetX) noexcept
		{
			const auto normalizedX = offsetX / deltaX;
			return firstY + deltaY * normalizedX;
		}

	private:
		// Linear shaper tests fail if intermediate value is stored as float.
		// Storing the coefficient as double prevents padding and gives up to 5% composition generation speedup.
		const double _c1;
		double _nextY;
	};

	// C2 = deltaY / deltaX^2
	// Y(X) = firstY + C2 * X^2
	// Y'(0) = 0
	class SmoothQuadraticShaper
	{
	public:
		explicit constexpr SmoothQuadraticShaper(const ShaperData& data) noexcept
			: _c0{ data._firstY }
			, _c2{ data._deltaY / (data._deltaX * data._deltaX) }
			, _nextX{ data._offsetX }
		{
		}

		constexpr float advance() noexcept
		{
			// Values can also be computed incrementally: Y(X + 1) = 2 * (Y(X) + C2) - Y(X - 1)
			// Unfortunately, float doesn't have enough precision, and double is much slower.
			const auto result = _c0 + _c2 * _nextX * _nextX;
			_nextX += 1;
			return result;
		}

		template <typename Float, typename = std::enable_if_t<std::is_floating_point_v<Float>>>
		static constexpr auto value(Float firstY, Float deltaY, Float deltaX, Float, Float offsetX) noexcept
		{
			const auto normalizedX = offsetX / deltaX;
			return firstY + deltaY * normalizedX * normalizedX;
		}

	private:
		const float _c0;
		const float _c2;
		float _nextX;
	};

	// C1 = 2 * deltaY / deltaX
	// C2 = deltaY / deltaX^2
	// Y(X) = firstY + (C1 - C2 * X) * X
	// Y'(deltaX) = 0
	class SharpQuadraticShaper
	{
	public:
		explicit constexpr SharpQuadraticShaper(const ShaperData& data) noexcept
			: _c0{ data._firstY }
			, _c1{ 2 * data._deltaY / data._deltaX }
			, _c2{ data._deltaY / (data._deltaX * data._deltaX) }
			, _nextX{ data._offsetX }
		{
		}

		constexpr auto advance() noexcept
		{
			// Values can also be computed incrementally: Y(X + 1) = 2 * (Y(X) - C2) - Y(X - 1)
			// Unfortunately, float doesn't have enough precision, and double is much slower.
			const auto result = _c0 + (_c1 - _c2 * _nextX) * _nextX;
			_nextX += 1;
			return result;
		}

		template <typename Float, typename = std::enable_if_t<std::is_floating_point_v<Float>>>
		static constexpr auto value(Float firstY, Float deltaY, Float deltaX, Float, Float offsetX) noexcept
		{
			const auto normalizedX = offsetX / deltaX;
			return firstY + deltaY * (2 - normalizedX) * normalizedX;
		}

	private:
		const float _c0;
		const float _c1;
		const float _c2;
		float _nextX;
	};

	// C2 = (3 - shape) * deltaY / deltaX^2
	// C3 = (2 - shape) * deltaY / deltaX^3
	// Y(X) = firstY + (C2 - C3 * X) * X^2
	// Y'(0) = 0
	// Y'(deltaX) = shape * deltaY / deltaX
	// 0 <= shape <= 3 : 0 <= |Y - firstY| <= |deltaY|
	class SmoothCubicShaper
	{
	public:
		static constexpr float kMinShape = 0;
		static constexpr float kMaxShape = 3;

		explicit constexpr SmoothCubicShaper(const ShaperData& data) noexcept
			: _c0{ data._firstY }
			, _c2{ (3 - data._shape) * data._deltaY / (data._deltaX * data._deltaX) }
			, _c3{ (2 - data._shape) * data._deltaY / (data._deltaX * data._deltaX * data._deltaX) }
			, _nextX{ data._offsetX }
		{
			assert(data._shape >= kMinShape && data._shape <= kMaxShape);
		}

		constexpr auto advance() noexcept
		{
			const auto result = _c0 + (_c2 - _c3 * _nextX) * _nextX * _nextX;
			_nextX += 1;
			return result;
		}

		template <typename Float, typename = std::enable_if_t<std::is_floating_point_v<Float>>>
		static constexpr auto value(Float firstY, Float deltaY, Float deltaX, Float shape, Float offsetX) noexcept
		{
			assert(shape >= kMinShape && shape <= kMaxShape);
			const auto normalizedX = offsetX / deltaX;
			return firstY + deltaY * (1 + (2 - shape) * (1 - normalizedX)) * normalizedX * normalizedX;
		}

	private:
		const float _c0;
		const float _c2;
		const float _c3;
		float _nextX;
	};

	// C1 = (3 - 2 * shape) * deltaY / deltaX
	// C2 = 6 * (1 - shape) * deltaY / deltaX^2
	// C3 = 4 * (1 - shape) * deltaY / deltaX^3
	// Y(X) = firstY + (C1 - (C2 - C3 * X) * X) * X
	// Y(deltaX / 2) = 0
	// Y'(deltaX / 2) = shape * deltaY / deltaX
	class SharpCubicShaper
	{
	public:
		// TODO: Find out actual shape limits.
		static constexpr float kMinShape = 0;
		static constexpr float kMaxShape = 1;

		explicit constexpr SharpCubicShaper(const ShaperData& data) noexcept
			: _c0{ data._firstY }
			, _c1{ (3 - 2 * data._shape) * data._deltaY / data._deltaX }
			, _c2{ 6 * (1 - data._shape) * data._deltaY / (data._deltaX * data._deltaX) }
			, _c3{ 4 * (1 - data._shape) * data._deltaY / (data._deltaX * data._deltaX * data._deltaX) }
			, _nextX{ data._offsetX }
		{
		}

		constexpr auto advance() noexcept
		{
			const auto result = _c0 + (_c1 - (_c2 - _c3 * _nextX) * _nextX) * _nextX;
			_nextX += 1;
			return result;
		}

		template <typename Float, typename = std::enable_if_t<std::is_floating_point_v<Float>>>
		static constexpr auto value(Float firstY, Float deltaY, Float deltaX, Float shape, Float offsetX) noexcept
		{
			assert(shape >= kMinShape && shape <= kMaxShape);
			const auto normalizedX = offsetX / deltaX;
			return firstY + deltaY * (1 + (1 - shape) * (2 - (6 - 4 * normalizedX) * normalizedX)) * normalizedX;
		}

	private:
		const float _c0;
		const float _c1;
		const float _c2;
		const float _c3;
		float _nextX;
	};

	// C2 = (15 - 8 * shape) * deltaY / deltaX^2
	// C3 = (50 - 32 * shape) * deltaY / deltaX^3
	// C4 = (60 - 40 * shape) * deltaY / deltaX^4
	// C5 = (24 - 16 * shape) * deltaY / deltaX^5
	// Y(X) = firstY + (C2 - (C3 - (C4 - C5 * X) * X) * X) * X^2
	// Y(deltaX / 2) = firstY + deltaY / 2
	// Y'(deltaX / 2) = shape * deltaY / deltaX
	class QuinticShaper
	{
	public:
		// TODO: Find out actual shape limits.
		static constexpr float kMinShape = -1;
		static constexpr float kMaxShape = 1;

		explicit constexpr QuinticShaper(const ShaperData& data) noexcept
			: _c0{ data._firstY }
			, _c2{ (15 - 8 * data._shape) * data._deltaY }
			, _c3{ (50 - 32 * data._shape) * data._deltaY }
			, _c4{ (60 - 40 * data._shape) * data._deltaY }
			, _c5{ (24 - 16 * data._shape) * data._deltaY }
			, _deltaX{ data._deltaX }
			, _nextX{ data._offsetX }
		{
			assert(data._shape >= kMinShape && data._shape <= kMaxShape);
		}

		constexpr float advance() noexcept
		{
			// The division is slow, but we can't store inverse deltaX because float doesn't have enough precision,
			// and storing it as double, while fixing the precision problem, makes it even more slower.
			const auto normalizedX = _nextX / _deltaX;
			const auto result = _c0 + (_c2 - (_c3 - (_c4 - _c5 * normalizedX) * normalizedX) * normalizedX) * normalizedX * normalizedX;
			_nextX += 1;
			return result;
		}

		template <typename Float, typename = std::enable_if_t<std::is_floating_point_v<Float>>>
		static constexpr auto value(Float firstY, Float deltaY, Float deltaX, Float shape, Float offsetX) noexcept
		{
			assert(shape >= kMinShape && shape <= kMaxShape);
			const auto normalizedX = offsetX / deltaX;
			return firstY + deltaY * (15 - 8 * shape - (50 - 32 * shape - (60 - 40 * shape - (24 - 16 * shape) * normalizedX) * normalizedX) * normalizedX) * normalizedX * normalizedX;
		}

	private:
		const float _c0;
		const float _c2;
		const float _c3;
		const float _c4;
		const float _c5;
		const float _deltaX;
		float _nextX;
	};

	// C(X) = deltaY * cos(pi * X / deltaX) / 2
	// Y(X) = firstY + deltaY / 2 - C(X)
	// C(X + 1) = 2 * cos(pi / deltaX) * C(X) - C(X - 1)
	class CosineShaper
	{
	public:
		explicit CosineShaper(const ShaperData& data) noexcept
		{
			const auto amplitude = data._deltaY / 2.;
			_base = data._firstY + amplitude;
			const auto theta = std::numbers::pi / data._deltaX;
			_multiplier = 2 * std::cos(theta);
			_lastCos = amplitude * std::cos(theta * data._offsetX - theta);
			_nextCos = amplitude * std::cos(theta * data._offsetX);
		}

		constexpr auto advance() noexcept
		{
			const auto result = _base - _nextCos;
			const auto nextCos = _multiplier * _nextCos - _lastCos;
			_lastCos = _nextCos;
			_nextCos = nextCos;
			return static_cast<float>(result);
		}

		template <typename Float, typename = std::enable_if_t<std::is_floating_point_v<Float>>>
		static auto value(Float firstY, Float deltaY, Float deltaX, Float, Float offsetX) noexcept
		{
			const auto normalizedX = offsetX / deltaX;
			return firstY + deltaY * (1 - std::cos(std::numbers::pi_v<Float> * normalizedX)) / 2;
		}

	private:
		double _base;
		double _multiplier;
		double _lastCos;
		double _nextCos;
	};
}
