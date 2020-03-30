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
	// Oscillators are stateful object which produce half-period wave parts.
	// An oscillation function F(X) must have the following properties:
	//   * F(0) = 1;
	//   * min <= F(X) <= 1 for 0 <= X < length.
	// An oscillator is parameterized with:
	//   * length of the half-period;
	//   * offset within the half-period to start generating wave at;
	//   * oscillation parameter which defines the minimum value of the oscillation function (min = 1 - 2 * oscillation),
	//     effectively blending the wave with rectangular wave of the same frequency.

	// C = 2 * oscillation / length
	// F(X) = 1 - C * X
	// F(X) = F(X - 1) - C
	class LinearOscillator
	{
	public:
		constexpr LinearOscillator(double length, double offset, double oscillation) noexcept
			: _coefficient{ 2 * oscillation / length }
			, _lastValue{ 1 - _coefficient * (offset - 1) }
		{
		}

		constexpr double operator()() noexcept
		{
			return _lastValue -= _coefficient;
		}

	private:
		const double _coefficient;
		double _lastValue;
	};

	// C = 2 * oscillation / length^2
	// F(X) = 1 - C * X^2
	// F(X) = F(X - 1) - C * (2 * X - 1)
	class QuadraticOscillator
	{
	public:
		constexpr QuadraticOscillator(double length, double offset, double oscillation) noexcept
			: _coefficient{ 2 * oscillation / (length * length) }
			, _lastX{ offset - 1 }
			, _lastValue{ 1 - _coefficient * _lastX * _lastX }
		{
		}

		constexpr double operator()() noexcept
		{
			_lastX += 1;
			return _lastValue -= _coefficient * (2 * _lastX - 1);
		}

	private:
		const double _coefficient;
		double _lastX;
		double _lastValue;
	};

	// C2 = 6 * oscillation * length^2
	// C3 = 4 * oscillation * length^3
	// F(X) = 1 - (C2 - C3 * X) * X^2
	// F(X) = F(X - 1) - [C2 * (2 * X - 1) - C3 * (3 * X * (X - 1) + 1)]
	class CubicOscillator
	{
	public:
		constexpr CubicOscillator(double length, double offset, double oscillation) noexcept
			: _coefficient2{ 6 * oscillation / (length * length) }
			, _coefficient3{ 4 * oscillation / (length * length * length) }
			, _lastX{ offset - 1 }
			, _lastValue{ 1 - (_coefficient2 - _coefficient3 * _lastX) * _lastX * _lastX }
		{
		}

		constexpr double operator()() noexcept
		{
			_lastX += 1;
			return _lastValue -= _coefficient2 * (2 * _lastX - 1) - _coefficient3 * (3 * _lastX * (_lastX - 1) + 1);
		}

	private:
		const double _coefficient2;
		const double _coefficient3;
		double _lastX;
		double _lastValue;
	};

	// F(X) = G(X) + (1 - oscillation)
	// G(X) = oscillation * cos(X * pi / length)
	// G(X) = [G(X - 1) - oscillation * sin(pi / length) * sin(X * pi / length)] / cos(pi / length)
	class CosineOscillator
	{
	public:
		CosineOscillator(double length, double offset, double oscillation) noexcept
			: _delta{ std::numbers::pi / length }
			, _cosDelta{ std::cos(_delta) }
			, _oscillationSinDelta{ oscillation * std::sin(_delta) }
			, _valueOffset{ 1 - oscillation }
			, _lastX{ offset - 1 }
			, _lastValue{ oscillation * std::cos(_delta * _lastX) }
		{
		}

		double operator()() noexcept
		{
			_lastX += 1;
			_lastValue = (_lastValue - _oscillationSinDelta * std::sin(_delta * _lastX)) / _cosDelta;
			return _lastValue + _valueOffset;
		}

	private:
		const double _delta;
		const double _cosDelta;
		const double _oscillationSinDelta;
		const double _valueOffset;
		double _lastX;
		double _lastValue;
	};
}
