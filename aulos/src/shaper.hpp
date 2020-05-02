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

	// C = deltaY / deltaX
	// Y(X) = firstY + C * X
	// Y(X + 1) = Y(X) + C
	class LinearShaper
	{
	public:
		constexpr LinearShaper(float firstY, float deltaY, float deltaX, float offsetX) noexcept
			: _coefficient{ deltaY / deltaX }
			, _nextY{ firstY + _coefficient * offsetX }
		{
		}

		constexpr auto advance() noexcept
		{
			const auto nextY = _nextY;
			_nextY += _coefficient;
			return static_cast<float>(nextY);
		}

		static constexpr auto value(float firstY, float deltaY, float deltaX, float offsetX) noexcept
		{
			const auto normalizedX = offsetX / deltaX;
			return firstY + deltaY * normalizedX;
		}

	private:
		// Linear shaper tests fail if intermediate value is stored as float.
		// Storing the coefficient as double prevents padding and gives up to 5% composition generation speedup.
		const double _coefficient;
		double _nextY;
	};

	// C = deltaY / deltaX^2
	// Y(X) = firstY + C * X^2
	// Y(X + 1) = Y(X) + C * (1 + 2 * X)
	// Y'(0) = 0
	class Quadratic1Shaper
	{
	public:
		constexpr Quadratic1Shaper(float firstY, float deltaY, float deltaX, float offsetX) noexcept
			: _coefficient{ deltaY / (deltaX * deltaX) }
			, _nextY{ firstY + _coefficient * offsetX * offsetX }
			, _nextX{ offsetX }
		{
		}

		constexpr auto advance() noexcept
		{
			const auto nextY = _nextY;
			_nextY += _coefficient * (1 + 2 * _nextX);
			_nextX += 1;
			return nextY;
		}

		static constexpr auto value(float firstY, float deltaY, float deltaX, float offsetX) noexcept
		{
			const auto normalizedX = offsetX / deltaX;
			return firstY + deltaY * normalizedX * normalizedX;
		}

	private:
		const float _coefficient;
		float _nextY;
		float _nextX;
	};

	// C1 = 2 * deltaY / deltaX
	// C2 = deltaY / deltaX^2
	// Y(X) = firstY + (C1 - C2 * X) * X
	// Y(X + 1) = Y(X) + C1 - C2 * (1 + 2 * X)
	// Y'(deltaX) = 0
	class Quadratic2Shaper
	{
	public:
		constexpr Quadratic2Shaper(float firstY, float deltaY, float deltaX, float offsetX) noexcept
			: _coefficient1{ 2 * deltaY / deltaX }
			, _coefficient2{ deltaY / (deltaX * deltaX) }
			, _nextY{ firstY + (_coefficient1 - _coefficient2 * offsetX) * offsetX }
			, _nextX{ offsetX }
		{
		}

		constexpr auto advance() noexcept
		{
			const auto nextY = _nextY;
			_nextY += _coefficient1 - _coefficient2 * (1 + 2 * _nextX);
			_nextX += 1;
			return nextY;
		}

		static constexpr auto value(float firstY, float deltaY, float deltaX, float offsetX) noexcept
		{
			const auto normalizedX = offsetX / deltaX;
			return firstY + deltaY * (2 - normalizedX) * normalizedX;
		}

	private:
		const float _coefficient1;
		const float _coefficient2;
		float _nextY;
		float _nextX;
	};

	// C2 = 3 * deltaY / deltaX^2
	// C3 = 2 * deltaY / deltaX^3
	// Y(X) = firstY + (C2 - C3 * X) * X^2
	// Y(X + 1) = Y(X) + C2 * (1 + 2 * X) - C3 * (1 + 3 * (1 + X) * X)
	// Y'(0) = 0
	// Y'(deltaX) = 0
	class CubicShaper
	{
	public:
		constexpr CubicShaper(float firstY, float deltaY, float deltaX, float offsetX) noexcept
			: _coefficient2{ 3 * deltaY / (deltaX * deltaX) }
			, _coefficient3{ 2 * deltaY / (deltaX * deltaX * deltaX) }
			, _nextY{ firstY + (_coefficient2 - _coefficient3 * offsetX) * offsetX * offsetX }
			, _nextX{ offsetX }
		{
		}

		constexpr auto advance() noexcept
		{
			const auto nextY = _nextY;
			_nextY += _coefficient2 * (2 * _nextX + 1) - _coefficient3 * (3 * _nextX * (_nextX + 1) + 1);
			_nextX += 1;
			return nextY;
		}

		static constexpr auto value(float firstY, float deltaY, float deltaX, float offsetX) noexcept
		{
			const auto normalizedX = offsetX / deltaX;
			return firstY + deltaY * (3 - 2 * normalizedX) * normalizedX * normalizedX;
		}

	private:
		const float _coefficient2;
		const float _coefficient3;
		float _nextY;
		float _nextX;
	};

	// C2 = 15 * deltaY / deltaX^2
	// C3 = 50 * deltaY / deltaX^3
	// C4 = 60 * deltaY / deltaX^4
	// C5 = 24 * deltaY / deltaX^5
	// Y(X) = firstY + (C2 - (C3 - (C4 - C5 * X) * X) * X) * X^2
	// Y(deltaX / 2) = firstY + deltaY / 2
	// Y'(deltaX / 2) = 0
	class QuinticShaper
	{
	public:
		constexpr QuinticShaper(float firstY, float deltaY, float deltaX, float offsetX) noexcept
			: _c0{ firstY }
			, _c2{ 15 * deltaY / (deltaX * deltaX) }
			, _c3{ 50 * deltaY / (deltaX * deltaX * deltaX) }
			, _c4{ 60 * deltaY / (deltaX * deltaX * deltaX * deltaX) }
			, _c5{ 24 * deltaY / (deltaX * deltaX * deltaX * deltaX * deltaX) }
			, _nextX{ offsetX }
			, _nextY{ firstY + (_c2 - (_c3 - (_c4 - _c5 * offsetX) * offsetX) * offsetX) * offsetX * offsetX }
		{
		}

		constexpr auto advance() noexcept
		{
			const auto nextY = _nextY;
			_nextX += 1;
			_nextY = _c0 + (_c2 - (_c3 - (_c4 - _c5 * _nextX) * _nextX) * _nextX) * _nextX * _nextX;
			return nextY;
		}

		static constexpr auto value(float firstY, float deltaY, float deltaX, float offsetX) noexcept
		{
			const auto normalizedX = offsetX / deltaX;
			return firstY + deltaY * (15 - (50 - (60 - 24 * normalizedX) * normalizedX) * normalizedX) * normalizedX * normalizedX;
		}

	private:
		const float _c0;
		const float _c2;
		const float _c3;
		const float _c4;
		const float _c5;
		float _nextX;
		float _nextY;
	};

	// Y(X) = G(X) + firstY + 0.5 * deltaY
	// G(X) = -0.5 * deltaY * cos(X * pi / deltaX)
	// G(X + 1) = G(X) * cos(pi / deltaX) + 0.5 * deltaY * sin(pi / deltaX) * sin(X * pi / deltaX)
	class CosineShaper
	{
	public:
		CosineShaper(float firstY, float deltaY, float deltaX, float offsetX) noexcept
			: _phi{ std::numbers::pi / deltaX }
			, _cosPhi{ std::cos(_phi) }
			, _scaledSinPhi{ .5 * deltaY * std::sin(_phi) }
			, _baseG{ firstY + .5 * deltaY }
			, _nextG{ -.5 * deltaY * std::cos(_phi * offsetX) }
			, _nextX{ offsetX }
		{
		}

		auto advance() noexcept
		{
			const auto nextG = _nextG;
			_nextG = _nextG * _cosPhi + _scaledSinPhi * std::sin(_phi * _nextX);
			_nextX += 1;
			return static_cast<float>(_baseG + nextG);
		}

		static auto value(float firstY, float deltaY, float deltaX, float offsetX) noexcept
		{
			const auto normalizedX = offsetX / deltaX;
			return firstY + deltaY * static_cast<float>(1 - std::cos(std::numbers::pi * normalizedX)) / 2;
		}

	private:
		const double _phi;
		const double _cosPhi;
		const double _scaledSinPhi;
		const double _baseG;
		double _nextG;
		float _nextX;
	};
}
