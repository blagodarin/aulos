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
	// Y(X) = Y(X - 1) + C
	class LinearShaper
	{
	public:
		constexpr LinearShaper(float firstY, float deltaY, float deltaX, float offsetX) noexcept
			: _coefficient{ deltaY / deltaX }
			, _lastValue{ firstY + _coefficient * (offsetX - 1) }
		{
		}

		constexpr auto advance() noexcept
		{
			return _lastValue += _coefficient;
		}

		static constexpr auto value(float firstY, float deltaY, float deltaX, float offsetX) noexcept
		{
			const auto normalizedX = offsetX / deltaX;
			return firstY + deltaY * normalizedX;
		}

	private:
		const float _coefficient;
		float _lastValue;
	};

	// C = deltaY / deltaX^2
	// Y(X) = firstY + C * X^2
	// Y(X) = Y(X - 1) + C * (2 * X - 1)
	// Y'(0) = 0
	class QuadraticShaper
	{
	public:
		constexpr QuadraticShaper(float firstY, float deltaY, float deltaX, float offsetX) noexcept
			: _coefficient{ deltaY / (deltaX * deltaX) }
			, _lastX{ offsetX - 1 }
			, _lastValue{ firstY + _coefficient * _lastX * _lastX }
		{
		}

		constexpr auto advance() noexcept
		{
			_lastX += 1;
			return _lastValue += _coefficient * (2 * _lastX - 1);
		}

		static constexpr auto value(float firstY, float deltaY, float deltaX, float offsetX) noexcept
		{
			const auto normalizedX = offsetX / deltaX;
			return firstY + deltaY * normalizedX * normalizedX;
		}

	private:
		const float _coefficient;
		float _lastX;
		float _lastValue;
	};

	// C2 = 3 * deltaY / deltaX^2
	// C3 = 2 * deltaY / deltaX^3
	// Y(X) = firstY + (C2 - C3 * X) * X^2
	// Y(X) = Y(X - 1) + [C2 * (2 * X - 1) - C3 * (3 * X * (X - 1) + 1)]
	// Y'(0) = 0
	// Y'(deltaX) = 0
	class CubicShaper
	{
	public:
		constexpr CubicShaper(float firstY, float deltaY, float deltaX, float offsetX) noexcept
			: _coefficient2{ 3 * deltaY / (deltaX * deltaX) }
			, _coefficient3{ 2 * deltaY / (deltaX * deltaX * deltaX) }
			, _lastX{ offsetX - 1 }
			, _lastValue{ firstY + (_coefficient2 - _coefficient3 * _lastX) * _lastX * _lastX }
		{
		}

		constexpr auto advance() noexcept
		{
			_lastX += 1;
			return _lastValue += _coefficient2 * (2 * _lastX - 1) - _coefficient3 * (3 * _lastX * (_lastX - 1) + 1);
		}

		static constexpr auto value(float firstY, float deltaY, float deltaX, float offsetX) noexcept
		{
			const auto normalizedX = offsetX / deltaX;
			return firstY + deltaY * normalizedX * normalizedX * (3 - 2 * normalizedX);
		}

	private:
		const float _coefficient2;
		const float _coefficient3;
		float _lastX;
		float _lastValue;
	};

	// Y(X) = G(X) + firstY + 0.5 * deltaY
	// G(X) = -0.5 * deltaY * cos(X * pi / deltaX)
	// G(X) = [G(X - 1) + 0.5 * deltaY * sin(pi / deltaX) * sin(X * pi / deltaX)] / cos(pi / deltaX)
	class CosineShaper
	{
	public:
		CosineShaper(float firstY, float deltaY, float deltaX, float offsetX) noexcept
			: _delta{ std::numbers::pi / deltaX }
			, _cosDelta{ std::cos(_delta) }
			, _scaledSinDelta{ -.5 * deltaY * std::sin(_delta) }
			, _valueOffset{ firstY + .5 * deltaY }
			, _lastX{ offsetX - 1 }
			, _lastValue{ -.5 * deltaY * std::cos(_delta * _lastX) }
		{
		}

		auto advance() noexcept
		{
			_lastX += 1;
			_lastValue = (_lastValue - _scaledSinDelta * std::sin(_delta * _lastX)) / _cosDelta;
			return static_cast<float>(_lastValue + _valueOffset);
		}

		static auto value(float firstY, float deltaY, float deltaX, float offsetX) noexcept
		{
			const auto normalizedX = offsetX / deltaX;
			return firstY + deltaY * static_cast<float>(1 - std::cos(std::numbers::pi * normalizedX)) / 2;
		}

	private:
		const double _delta;
		const double _cosDelta;
		const double _scaledSinDelta;
		const double _valueOffset;
		float _lastX;
		double _lastValue;
	};
}
