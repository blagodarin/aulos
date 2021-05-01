// This file is part of the Aulos toolkit.
// Copyright (C) Sergei Blagodarin.
// SPDX-License-Identifier: Apache-2.0

#include <aulos/src/voice.hpp>

#include <doctest.h>

using namespace std::chrono_literals;

namespace
{
	constexpr auto kTestSamplingRate = 44'000u;
	constexpr auto kTestNoteFrequency = 440;
	constexpr auto kPeriodSamples = kTestSamplingRate / kTestNoteFrequency;

	template <typename Shaper>
	struct MonoVoice
	{
		const aulos::WaveData _waveData;
		aulos::MonoVoice<Shaper> _voice;

		MonoVoice(const aulos::VoiceData& data, float amplitude)
			: _waveData{ data, kTestSamplingRate }
			, _voice{ _waveData, kTestSamplingRate }
		{
			_voice.start(kTestNoteFrequency, amplitude, 0);
		}

		auto render()
		{
			float sample = 0.f;
			_voice.render(&sample, 1);
			return sample;
		}
	};

	template <typename Shaper>
	struct StereoVoice
	{
		const aulos::WaveData _waveData;
		aulos::StereoVoice<Shaper> _voice;

		StereoVoice(const aulos::VoiceData& data, float amplitude)
			: _waveData{ data, kTestSamplingRate }
			, _voice{ _waveData, kTestSamplingRate }
		{
			_voice.start(kTestNoteFrequency, amplitude, 0);
		}

		auto render()
		{
			std::pair<float, float> frame{ 0.f, 0.f };
			_voice.render(&frame.first, 1);
			return frame;
		}
	};
}

TEST_CASE("MonoVoice (sawtooth wave)")
{
	aulos::VoiceData data;
	data._amplitudeEnvelope._changes.emplace_back(0ms, 1.f);
	data._amplitudeEnvelope._changes.emplace_back(500ms, 1.f);
	data._asymmetryEnvelope._changes.emplace_back(0ms, 1.f);

	constexpr auto amplitude = .1f;
	MonoVoice<aulos::LinearShaper> voice{ data, amplitude };

	// The first period.
	auto sample = voice.render();
	auto expected = 0.f;
	CHECK(sample == expected);
	for (unsigned i = 1; i < kPeriodSamples; ++i)
	{
		sample = voice.render();
		CHECK(sample > 0.f);
		CHECK(sample < amplitude);
		expected += amplitude / kPeriodSamples;
		CHECK(sample == doctest::Approx{ expected });
	}

	// The second period.
	sample = voice.render();
	expected = -amplitude;
	CHECK(sample == expected);
	for (unsigned i = 1; i < kPeriodSamples; ++i)
	{
		sample = voice.render();
		CHECK(sample > -amplitude);
		CHECK(sample < amplitude);
		expected += 2 * amplitude / kPeriodSamples;
		CHECK(sample == doctest::Approx{ expected });
	}
}

TEST_CASE("StereoVoice (sawtooth wave)")
{
	aulos::VoiceData data;
	data._amplitudeEnvelope._changes.emplace_back(0ms, 1.f);
	data._amplitudeEnvelope._changes.emplace_back(500ms, 1.f);
	data._asymmetryEnvelope._changes.emplace_back(0ms, 1.f);

	constexpr auto amplitude = .1f;
	StereoVoice<aulos::LinearShaper> voice{ data, amplitude };

	// The first period.
	auto frame = voice.render();
	CHECK(frame.first == frame.second);
	auto expected = 0.f;
	CHECK(frame.first == expected);
	for (unsigned i = 1; i < kPeriodSamples; ++i)
	{
		frame = voice.render();
		CHECK(frame.first == frame.second);
		CHECK(frame.first > 0.f);
		CHECK(frame.first < amplitude);
		expected += amplitude / kPeriodSamples;
		CHECK(frame.first == doctest::Approx{ expected });
	}

	// The second period.
	frame = voice.render();
	CHECK(frame.first == frame.second);
	expected = -amplitude;
	CHECK(frame.first == expected);
	for (unsigned i = 1; i < kPeriodSamples; ++i)
	{
		frame = voice.render();
		CHECK(frame.first == frame.second);
		CHECK(frame.first > -amplitude);
		CHECK(frame.first < amplitude);
		expected += 2 * amplitude / kPeriodSamples;
		CHECK(frame.first == doctest::Approx{ expected });
	}
}

TEST_CASE("MonoVoice (square wave)")
{
	aulos::VoiceData data;
	data._amplitudeEnvelope._changes.emplace_back(0ms, 1.f);
	data._amplitudeEnvelope._changes.emplace_back(500ms, 1.f);
	data._oscillationEnvelope._changes.emplace_back(0ms, 1.f);

	constexpr auto amplitude = .2f;
	constexpr auto partLength = kPeriodSamples / 2;
	MonoVoice<aulos::LinearShaper> voice{ data, amplitude };

	// The first period.
	for (unsigned i = 0; i < partLength; ++i)
		CHECK(voice.render() == amplitude);
	for (unsigned i = 0; i < partLength; ++i)
		CHECK(voice.render() == -amplitude);

	// The second period.
	CHECK(voice.render() == amplitude);
}

TEST_CASE("MonoVoice (triangle wave)")
{
	aulos::VoiceData data;
	data._amplitudeEnvelope._changes.emplace_back(0ms, 1.f);
	data._amplitudeEnvelope._changes.emplace_back(500ms, 1.f);

	constexpr auto amplitude = .3f;
	constexpr auto partLength = kPeriodSamples / 2;
	MonoVoice<aulos::LinearShaper> voice{ data, amplitude };

	// The first period.
	auto sample = voice.render();
	auto expected = 0.f;
	CHECK(sample == expected);
	for (unsigned i = 1; i < partLength; ++i)
	{
		sample = voice.render();
		CHECK(sample > 0.f);
		CHECK(sample < amplitude);
		expected += amplitude / partLength;
		CHECK(sample == doctest::Approx{ expected });
	}
	sample = voice.render();
	expected = amplitude;
	CHECK(sample == expected);
	for (unsigned i = 1; i < partLength; ++i)
	{
		sample = voice.render();
		CHECK(sample < amplitude);
		CHECK(sample > -amplitude);
		expected -= (2 * amplitude) / partLength;
		CHECK(sample == doctest::Approx{ expected });
	}

	// The second period.
	sample = voice.render();
	expected = -amplitude;
	CHECK(sample == expected);
	for (unsigned i = 1; i < partLength; ++i)
	{
		sample = voice.render();
		CHECK(sample > -amplitude);
		CHECK(sample < amplitude);
		expected += (2 * amplitude) / partLength;
		CHECK(sample == doctest::Approx{ expected });
	}
	CHECK(voice.render() == amplitude);
}

TEST_CASE("MonoVoice (asymmetric triangle wave)")
{
	aulos::VoiceData data;
	data._amplitudeEnvelope._changes.emplace_back(0ms, 1.f);
	data._amplitudeEnvelope._changes.emplace_back(500ms, 1.f);
	data._asymmetryEnvelope._changes.emplace_back(0ms, .5f);

	constexpr auto amplitude = .4f;
	const auto firstPartLength = kPeriodSamples * 3 / 4;
	const auto secondPartLength = kPeriodSamples - firstPartLength;
	MonoVoice<aulos::LinearShaper> voice{ data, amplitude };

	// The first period.
	auto sample = voice.render();
	auto expected = 0.f;
	CHECK(sample == expected);
	for (unsigned i = 1; i < firstPartLength; ++i)
	{
		sample = voice.render();
		CHECK(sample > 0.f);
		CHECK(sample < amplitude);
		expected += amplitude / firstPartLength;
		CHECK(sample == doctest::Approx{ expected });
	}
	sample = voice.render();
	expected = amplitude;
	CHECK(sample == expected);
	for (unsigned i = 1; i < secondPartLength; ++i)
	{
		sample = voice.render();
		CHECK(sample < amplitude);
		CHECK(sample > -amplitude);
		expected -= (2 * amplitude) / secondPartLength;
		CHECK(sample == doctest::Approx{ expected });
	}

	// The second period.
	sample = voice.render();
	expected = -amplitude;
	CHECK(sample == expected);
	for (unsigned i = 1; i < firstPartLength; ++i)
	{
		sample = voice.render();
		CHECK(sample > -amplitude);
		CHECK(sample < amplitude);
		expected += (2 * amplitude) / firstPartLength;
		CHECK(sample == doctest::Approx{ expected });
	}
	CHECK(voice.render() == amplitude);
}
