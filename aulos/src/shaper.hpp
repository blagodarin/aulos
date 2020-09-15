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

	// Y'(0) = shape * deltaY / deltaX
	// Y(deltaX / 2) = firstY + deltaY / 2
	// Y'(deltaX) = shape * deltaY / deltaX
	class QuadraticShaper
	{
	public:
		// The shape parameter defines the curve shape as follows:
		//  * [0.00, 1.00] - the function is monotonic and gradually transforms from quadratic with zero derivatives at the ends to linear;
		//  * (1.00, 2.00] - the function is monotonic and gradually transforms from linear to quadratic with zero derivative in the middle;
		//  * (2.00, 6.82] - the function is non-monotonic with two distinct extrema in the range which touch Y limits at 4+2*sqrt(2).
		static constexpr float kMinShape = 0.f;
		static constexpr float kMaxShape = 6.82f;

		explicit constexpr QuadraticShaper(const ShaperData& data) noexcept
			: _quadratic{ (1 - data._shape) * data._deltaY / 2 }
			, _linear0{ data._firstY - _quadratic }
			, _linear1{ data._deltaY / 2 + _quadratic }
			, _halfDeltaX{ data._deltaX / 2 }
			, _nextX{ data._offsetX }
		{
		}

		constexpr float advance() noexcept
		{
			const auto x = _nextX / _halfDeltaX;
			const auto quadratic = (_nextX < _halfDeltaX ? _quadratic : -_quadratic) * (1 - x) * (1 - x);
			_nextX += 1;
			return _linear0 + _linear1 * x + quadratic;
		}

		template <typename Float, typename = std::enable_if_t<std::is_floating_point_v<Float>>>
		static constexpr auto value(Float firstY, Float deltaY, Float deltaX, Float shape, Float offsetX) noexcept
		{
			const auto normalizedX = offsetX / deltaX;
			return firstY + deltaY * (offsetX < deltaX / 2 ? (shape + 2 * (1 - shape) * normalizedX) * normalizedX : (shape - 1) * (1 + 2 * normalizedX * normalizedX) + (4 - 3 * shape) * normalizedX);
		}

	private:
		const float _quadratic;
		const float _linear0;
		const float _linear1;
		const float _halfDeltaX;
		float _nextX;
	};

	// C1 = S1 * deltaY / deltaX
	// C2 = (2 * S1 + S2 - 3) * deltaY / deltaX^2
	// C3 = (S1 + S2 - 1) * deltaY / deltaX^3
	// Y(X) = firstY + (C1 - (C2 - C3 * X) * X) * X
	// Y'(0) = S1 * deltaY / deltaX
	// Y'(deltaX) = S2 * deltaY / deltaX
	class CubicShaper
	{
	public:
		// The shape parameter defines the curve shape as follows:
		//  * [0, 1] - the function is monotonic and gradually transforms from cubic with zero derivatives at the ends to linear;
		//  * (1, 3] - the function is monotonic and gradually transforms from linear to cubic with zero derivative in the middle;
		//  * (3, 9] - the function is non-monotonic with two distinct extrema in the range which touch Y limits at 9.
		static constexpr float kMinShape = 0.f;
		static constexpr float kMaxShape = 8.98f; // Float precision is insufficient to satisfy Y range constraints at the precise maximum.

		explicit constexpr CubicShaper(const ShaperData& data) noexcept
			: _c0{ data._firstY }
			, _c1{ data._shape /*1*/ * data._deltaY / data._deltaX }
			, _c2{ (2 * data._shape /*1*/ + data._shape /*2*/ - 3) * data._deltaY / (data._deltaX * data._deltaX) }
			, _c3{ (data._shape /*1*/ + data._shape /*2*/ - 2) * data._deltaY / (data._deltaX * data._deltaX * data._deltaX) }
			, _nextX{ data._offsetX }
		{
			assert(data._shape >= kMinShape && data._shape <= kMaxShape);
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
			return firstY + deltaY * (shape /*1*/ - ((2 * shape /*1*/ + shape /*2*/ - 3) - (shape /*1*/ + shape /*2*/ - 2) * normalizedX) * normalizedX) * normalizedX;
		}

	private:
		const float _c0;
		const float _c1;
		const float _c2;
		const float _c3;
		float _nextX;
	};

	// C2 = (15 + 8 * shape) * deltaY / deltaX^2
	// C3 = (50 + 32 * shape) * deltaY / deltaX^3
	// C4 = (60 + 40 * shape) * deltaY / deltaX^4
	// C5 = (24 + 16 * shape) * deltaY / deltaX^5
	// Y(X) = firstY + (C2 - (C3 - (C4 - C5 * X) * X) * X) * X^2
	// Y(deltaX / 2) = firstY + deltaY / 2
	// Y'(deltaX / 2) = -shape * deltaY / deltaX
	class QuinticShaper
	{
	public:
		// The shape parameter defines the curve shape as follows:
		//  * [-1.5, 0.000] - the function is monotonic and gradually transforms from smooth cubic to quintic with zero derivative in the middle;
		//  * ( 0.0, 4.045] - the function is non-monotonic with two distinct extrema in the range which touch Y limits at (4016+3025*sqrt(110))/8836.
		static constexpr float kMinShape = -1.5f;
		static constexpr float kMaxShape = 4.01f; // More precise maximum breaks Y range constraints.

		explicit constexpr QuinticShaper(const ShaperData& data) noexcept
			: _c0{ data._firstY }
			, _c2{ (15 + 8 * data._shape) * data._deltaY }
			, _c3{ (50 + 32 * data._shape) * data._deltaY }
			, _c4{ (60 + 40 * data._shape) * data._deltaY }
			, _c5{ (24 + 16 * data._shape) * data._deltaY }
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
			return firstY + deltaY * (15 + 8 * shape - (50 + 32 * shape - (60 + 40 * shape - (24 + 16 * shape) * normalizedX) * normalizedX) * normalizedX) * normalizedX * normalizedX;
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
