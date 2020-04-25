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
	// Generators are stateful objects which produce half-period wave parts.
	//
	// A generator is parameterized with:
	//   * length of the half-period;
	//   * offset within the half-period to start generating wave at;
	//   * amplitude of the generated wave, i. e. maximum value of the generator function in the half-period;
	//   * oscillation parameter which defines the minimum value of the generator function in the half-period,
	//     effectively blending the wave with rectangular wave of the same frequency.
	//
	// The parameters have the following properties:
	//   * length > 0;
	//   * 0 <= offset < length;
	//   * 0 <= oscillation <= amplitude;
	//   * minimum = amplitude - 2 * oscillation.
	//
	// The resulting generator function F(X) must have the following properties:
	//   * F(0) = amplitude;
	//   * minimum <= F(X) <= amplitude for 0 <= X < length.

	// C = 2 * oscillation / length
	// F(X) = amplitude - C * X
	// F(X) = F(X - 1) - C
	class LinearGenerator
	{
	public:
		constexpr LinearGenerator(double length, double offset, double amplitude, double oscillation) noexcept
			: _coefficient{ 2 * oscillation / length }
			, _lastValue{ amplitude - _coefficient * (offset - 1) }
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
	// F(X) = amplitude - C * X^2
	// F(X) = F(X - 1) - C * (2 * X - 1)
	class QuadraticGenerator
	{
	public:
		constexpr QuadraticGenerator(double length, double offset, double amplitude, double oscillation) noexcept
			: _coefficient{ 2 * oscillation / (length * length) }
			, _lastX{ offset - 1 }
			, _lastValue{ amplitude - _coefficient * _lastX * _lastX }
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
	// F(X) = amplitude - (C2 - C3 * X) * X^2
	// F(X) = F(X - 1) - [C2 * (2 * X - 1) - C3 * (3 * X * (X - 1) + 1)]
	class CubicGenerator
	{
	public:
		constexpr CubicGenerator(double length, double offset, double amplitude, double oscillation) noexcept
			: _coefficient2{ 6 * oscillation / (length * length) }
			, _coefficient3{ 4 * oscillation / (length * length * length) }
			, _lastX{ offset - 1 }
			, _lastValue{ amplitude - (_coefficient2 - _coefficient3 * _lastX) * _lastX * _lastX }
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

	// F(X) = G(X) + amplitude - oscillation
	// G(X) = oscillation * cos(X * pi / length)
	// G(X) = [G(X - 1) - oscillation * sin(pi / length) * sin(X * pi / length)] / cos(pi / length)
	class CosineGenerator
	{
	public:
		CosineGenerator(double length, double offset, double amplitude, double oscillation) noexcept
			: _delta{ std::numbers::pi / length }
			, _cosDelta{ std::cos(_delta) }
			, _scaledSinDelta{ oscillation * std::sin(_delta) }
			, _valueOffset{ amplitude - oscillation }
			, _lastX{ offset - 1 }
			, _lastValue{ oscillation * std::cos(_delta * _lastX) }
		{
		}

		double operator()() noexcept
		{
			_lastX += 1;
			_lastValue = (_lastValue - _scaledSinDelta * std::sin(_delta * _lastX)) / _cosDelta;
			return _lastValue + _valueOffset;
		}

	private:
		const double _delta;
		const double _cosDelta;
		const double _scaledSinDelta;
		const double _valueOffset;
		double _lastX;
		double _lastValue;
	};
}
