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

#include <cmath>
#include <numbers>

namespace aulos
{
	// Shaper is a stateful object that advances from (0, firstY) to (deltaX, firstY + deltaY) according to the shape function Y(X)
	// which stays in [firstY, firstY + deltaY] (or [firstY + deltaY, firstY] if deltaY is negative) for any X in [0, deltaX].
	// Shapers start at offsetX which must be in [0, deltaX).

	// C1 = deltaY / deltaX
	// Y(X) = firstY + C1 * X
	// Y(X + 1) = Y(X) + C1
	class LinearShaper
	{
	public:
		constexpr LinearShaper(float firstY, float deltaY, float deltaX, float, float offsetX) noexcept
			: _c1{ deltaY / deltaX }
			, _nextY{ firstY + _c1 * offsetX }
		{
		}

		constexpr auto advance() noexcept
		{
			const auto nextY = _nextY;
			_nextY += _c1;
			return static_cast<float>(nextY);
		}

		static constexpr auto value(float firstY, float deltaY, float deltaX, float, float offsetX) noexcept
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
	class Quadratic1Shaper
	{
	public:
		constexpr Quadratic1Shaper(float firstY, float deltaY, float deltaX, float, float offsetX) noexcept
			: _c0{ firstY }
			, _c2{ deltaY / (deltaX * deltaX) }
			, _nextX{ offsetX }
			, _nextY{ firstY + _c2 * offsetX * offsetX }
		{
		}

		constexpr auto advance() noexcept
		{
			const auto nextY = _nextY;
			_nextX += 1;
			_nextY = _c0 + _c2 * _nextX * _nextX;
			return nextY;
		}

		static constexpr auto value(float firstY, float deltaY, float deltaX, float, float offsetX) noexcept
		{
			const auto normalizedX = offsetX / deltaX;
			return firstY + deltaY * normalizedX * normalizedX;
		}

	private:
		const float _c0;
		const float _c2;
		float _nextX;
		float _nextY;
	};

	// C1 = 2 * deltaY / deltaX
	// C2 = deltaY / deltaX^2
	// Y(X) = firstY + (C1 - C2 * X) * X
	// Y'(deltaX) = 0
	class Quadratic2Shaper
	{
	public:
		constexpr Quadratic2Shaper(float firstY, float deltaY, float deltaX, float, float offsetX) noexcept
			: _c0{ firstY }
			, _c1{ 2 * deltaY / deltaX }
			, _c2{ deltaY / (deltaX * deltaX) }
			, _nextX{ offsetX }
			, _nextY{ firstY + (_c1 - _c2 * offsetX) * offsetX }
		{
		}

		constexpr auto advance() noexcept
		{
			const auto nextY = _nextY;
			_nextX += 1;
			_nextY = _c0 + (_c1 - _c2 * _nextX) * _nextX;
			return nextY;
		}

		static constexpr auto value(float firstY, float deltaY, float deltaX, float, float offsetX) noexcept
		{
			const auto normalizedX = offsetX / deltaX;
			return firstY + deltaY * (2 - normalizedX) * normalizedX;
		}

	private:
		const float _c0;
		const float _c1;
		const float _c2;
		float _nextX;
		float _nextY;
	};

	// C2 = 3 * deltaY / deltaX^2
	// C3 = 2 * deltaY / deltaX^3
	// Y(X) = firstY + (C2 - C3 * X) * X^2
	// Y'(0) = 0
	// Y'(deltaX) = 0
	class CubicShaper
	{
	public:
		constexpr CubicShaper(float firstY, float deltaY, float deltaX, float, float offsetX) noexcept
			: _c0{ firstY }
			, _c2{ 3 * deltaY / (deltaX * deltaX) }
			, _c3{ 2 * deltaY / (deltaX * deltaX * deltaX) }
			, _nextX{ offsetX }
			, _nextY{ firstY + (_c2 - _c3 * offsetX) * offsetX * offsetX }
		{
		}

		constexpr auto advance() noexcept
		{
			const auto nextY = _nextY;
			_nextX += 1;
			_nextY = _c0 + (_c2 - _c3 * _nextX) * _nextX * _nextX;
			return nextY;
		}

		static constexpr auto value(float firstY, float deltaY, float deltaX, float, float offsetX) noexcept
		{
			const auto normalizedX = offsetX / deltaX;
			return firstY + deltaY * (3 - 2 * normalizedX) * normalizedX * normalizedX;
		}

	private:
		const float _c0;
		const float _c2;
		const float _c3;
		float _nextX;
		float _nextY;
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
		constexpr QuinticShaper(float firstY, float deltaY, float deltaX, float shape, float offsetX) noexcept
			: _c0{ firstY }
			, _c2{ (15 - 8 * shape) * deltaY }
			, _c3{ (50 - 32 * shape) * deltaY }
			, _c4{ (60 - 40 * shape) * deltaY }
			, _c5{ (24 - 16 * shape) * deltaY }
			, _deltaX{ deltaX }
			, _nextX{ offsetX - 1 }
		{
			advance();
		}

		constexpr float advance() noexcept
		{
			const auto nextY = _nextY;
			_nextX += 1;
			// The division is slow, but we can't store inverse deltaX because float doesn't have enough precision,
			// and storing it as double, while fixing the precision problem, makes it even more slower.
			const auto normalizedX = _nextX / _deltaX;
			_nextY = _c0 + (_c2 - (_c3 - (_c4 - _c5 * normalizedX) * normalizedX) * normalizedX) * normalizedX * normalizedX;
			return nextY;
		}

		static constexpr auto value(float firstY, float deltaY, float deltaX, float shape, float offsetX) noexcept
		{
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
		float _nextY = 0;
	};

	// Y(X) = firstY + deltaY * (1 - cos(pi * X / deltaX)) / 2
	class CosineShaper
	{
	public:
		CosineShaper(float firstY, float deltaY, float deltaX, float, float offsetX) noexcept
			: _c1{ deltaY / 2 }
			, _c0{ firstY + _c1 }
			, _phi{ std::numbers::pi_v<float> / deltaX }
			, _nextX{ offsetX }
			, _nextY{ _c0 - _c1 * std::cos(_phi * offsetX) }
		{
		}

		auto advance() noexcept
		{
			const auto nextY = _nextY;
			_nextX += 1;
			_nextY = _c0 - _c1 * std::cos(_phi * _nextX);
			return nextY;
		}

		static auto value(float firstY, float deltaY, float deltaX, float, float offsetX) noexcept
		{
			const auto normalizedX = offsetX / deltaX;
			return firstY + deltaY * (1 - std::cos(std::numbers::pi_v<float> * normalizedX)) / 2;
		}

	private:
		const float _c1;
		const float _c0;
		const float _phi;
		float _nextX;
		float _nextY;
	};
}
