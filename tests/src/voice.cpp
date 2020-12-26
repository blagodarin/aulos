// This file is part of the Aulos toolkit.
// Copyright (C) Sergei Blagodarin.
// SPDX-License-Identifier: Apache-2.0

#include <aulos/src/voice.hpp>

#include <doctest.h>

using namespace std::chrono_literals;

namespace
{
	constexpr auto kTestSamplingRate = 44'000u;
	constexpr auto kTestNote = aulos::Note::A4; // A4 note frequency is exactly 440 Hz, so the period should be exactly 100 frames.

	template <typename Shaper>
	struct MonoTester
	{
		const aulos::WaveData _waveData;
		aulos::MonoVoice<Shaper> _voice;

		MonoTester(const aulos::VoiceData& data, float amplitude)
			: _waveData{ data, kTestSamplingRate }
			, _voice{ _waveData, kTestSamplingRate }
		{
			_voice.start(kTestNote, amplitude, 0);
		}

		auto render()
		{
			float sample = 0.f;
			_voice.render(&sample, 1);
			return sample;
		}
	};

	template <typename Shaper>
	struct StereoTester
	{
		const aulos::WaveData _waveData;
		aulos::StereoVoice<Shaper> _voice;

		StereoTester(const aulos::VoiceData& data, float amplitude)
			: _waveData{ data, kTestSamplingRate }
			, _voice{ _waveData, kTestSamplingRate }
		{
			_voice.start(kTestNote, amplitude, 0);
		}

		auto render()
		{
			std::pair<float, float> frame{ 0.f, 0.f };
			_voice.render(&frame.first, 1);
			return frame;
		}
	};
}

TEST_CASE("wave_sawtooth_mono")
{
	aulos::VoiceData data;
	data._amplitudeEnvelope._changes.emplace_back(0ms, 1.f);
	data._amplitudeEnvelope._changes.emplace_back(500ms, 1.f);
	data._asymmetryEnvelope._changes.emplace_back(0ms, 1.f);

	constexpr auto amplitude = .1f;
	MonoTester<aulos::LinearShaper> tester{ data, amplitude };
	auto sample = tester.render();
	CHECK(sample == amplitude);
	for (int i = 1; i < 100; ++i)
	{
		const auto nextSample = tester.render();
		CHECK(nextSample > -amplitude);
		CHECK(sample > nextSample);
		sample = nextSample;
	}
	CHECK(tester.render() == amplitude);
}

TEST_CASE("wave_sawtooth_stereo")
{
	aulos::VoiceData data;
	data._amplitudeEnvelope._changes.emplace_back(0ms, 1.f);
	data._amplitudeEnvelope._changes.emplace_back(500ms, 1.f);
	data._asymmetryEnvelope._changes.emplace_back(0ms, 1.f);

	constexpr auto amplitude = .1f;
	StereoTester<aulos::LinearShaper> tester{ data, amplitude };
	auto frame = tester.render();
	CHECK(frame.first == amplitude);
	CHECK(frame.second == amplitude);
	for (int i = 1; i < 100; ++i)
	{
		const auto nextFrame = tester.render();
		CHECK(nextFrame.first > -amplitude);
		CHECK(frame.first > nextFrame.first);
		CHECK(nextFrame.second > -amplitude);
		CHECK(frame.second > nextFrame.second);
		frame = nextFrame;
	}
	frame = tester.render();
	CHECK(frame.first == amplitude);
	CHECK(frame.second == amplitude);
}

TEST_CASE("wave_square_mono")
{
	aulos::VoiceData data;
	data._amplitudeEnvelope._changes.emplace_back(0ms, 1.f);
	data._amplitudeEnvelope._changes.emplace_back(500ms, 1.f);
	data._oscillationEnvelope._changes.emplace_back(0ms, 1.f);

	constexpr auto amplitude = .2f;
	MonoTester<aulos::LinearShaper> tester{ data, amplitude };
	for (int i = 0; i < 50; ++i)
		CHECK(tester.render() == amplitude);
	for (int i = 0; i < 50; ++i)
		CHECK(tester.render() == -amplitude);
	CHECK(tester.render() == amplitude);
}

TEST_CASE("wave_triangle_mono")
{
	aulos::VoiceData data;
	data._amplitudeEnvelope._changes.emplace_back(0ms, 1.f);
	data._amplitudeEnvelope._changes.emplace_back(500ms, 1.f);

	constexpr auto amplitude = .3f;
	MonoTester<aulos::LinearShaper> tester{ data, amplitude };
	auto sample = tester.render();
	CHECK(sample == amplitude);
	for (int i = 1; i < 50; ++i)
	{
		const auto nextSample = tester.render();
		CHECK(nextSample > -amplitude);
		CHECK(sample > nextSample);
		sample = nextSample;
	}
	sample = tester.render();
	CHECK(sample == -amplitude);
	for (int i = 1; i < 25; ++i)
	{
		const auto nextSample = tester.render();
		CHECK(nextSample < 0.f);
		CHECK(sample < nextSample);
		sample = nextSample;
	}
}

TEST_CASE("wave_triangle_asymmetric_mono")
{
	aulos::VoiceData data;
	data._amplitudeEnvelope._changes.emplace_back(0ms, 1.f);
	data._amplitudeEnvelope._changes.emplace_back(500ms, 1.f);
	data._asymmetryEnvelope._changes.emplace_back(0ms, .5f);

	constexpr auto amplitude = .4f;
	MonoTester<aulos::LinearShaper> tester{ data, amplitude };
	auto sample = tester.render();
	CHECK(sample == amplitude);
	for (int i = 1; i < 75; ++i)
	{
		const auto nextSample = tester.render();
		CHECK(nextSample > -amplitude);
		CHECK(sample > nextSample);
		sample = nextSample;
	}
	sample = tester.render();
	CHECK(sample == -amplitude);
	for (int i = 1; i < 25; ++i)
	{
		const auto nextSample = tester.render();
		CHECK(nextSample < amplitude);
		CHECK(sample < nextSample);
		sample = nextSample;
	}
	CHECK(tester.render() == amplitude);
}
