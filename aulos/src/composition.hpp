// This file is part of the Aulos toolkit.
// Copyright (C) Sergei Blagodarin.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <aulos/composition.hpp>
#include <aulos/common.hpp>

#include <cmath>
#include <string>

namespace aulos
{
	struct Fragment
	{
		size_t _delay = 0;
		size_t _sequence = 0;

		constexpr Fragment(size_t delay, size_t sequence) noexcept
			: _delay{ delay }, _sequence{ sequence } {}
	};

	struct Track
	{
		TrackProperties _properties;
		std::vector<std::vector<Sound>> _sequences;
		std::vector<Fragment> _fragments;
	};

	struct Part
	{
		VoiceData _voice;
		std::string _voiceName;
		std::vector<Track> _tracks;
	};

	template <typename T, unsigned kFractionBits>
	class Fixed
	{
	public:
		constexpr Fixed() noexcept = default;
		constexpr explicit Fixed(float value) noexcept
			: _value{ static_cast<T>(value / kOne) } {}

		constexpr explicit operator float() const noexcept { return static_cast<float>(_value) / kOne; }

		constexpr T store() const noexcept { return _value; }

		static Fixed ceil(float value) noexcept { return Fixed{ static_cast<T>(std::ceil(value * kOne)) }; }
		static constexpr Fixed load(T value) noexcept { return Fixed{ value }; }

	private:
		T _value = 0;
		static constexpr T kOne = 1 << kFractionBits;
		constexpr explicit Fixed(T value) noexcept
			: _value{ value } {}
	};

	using Fixed12u4 = Fixed<uint16_t, 4>;

	struct CompositionImpl final : public Composition
	{
		unsigned _speed = kMinSpeed;
		unsigned _loopOffset = 0;
		unsigned _loopLength = 0;
		Fixed12u4 _gainDivisor{ 1.f };
		std::vector<Part> _parts;
		std::string _title;
		std::string _author;

		bool hasLoop() const noexcept override { return _loopLength > 0; }

		void load(const char* source);
	};
}
