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

#include <aulos/data.hpp>

#include <doctest.h>

namespace
{
	struct TestVoice
	{
		const std::unique_ptr<aulos::VoiceRenderer> _renderer;

		TestVoice(const aulos::VoiceData& data, float amplitude)
			: _renderer{ aulos::VoiceRenderer::create(data, 44'000, 1) }
		{
			REQUIRE(_renderer);
			CHECK(_renderer->channels() == 1);
			CHECK(_renderer->samplingRate() == 44'000);
			CHECK(_renderer->totalSamples() == 22'000);
			_renderer->start(aulos::Note::A4, amplitude); // A4 note frequency is exactly 440 Hz, so the period should be exactly 100 samples.
		}

		auto renderSample()
		{
			float sample = 0.f;
			REQUIRE(_renderer->render(&sample, sizeof sample) == sizeof sample);
			return sample;
		}
	};
}

TEST_CASE("wave_sawtooth")
{
	aulos::VoiceData data;
	data._amplitudeEnvelope._initial = 1.f;
	data._amplitudeEnvelope._changes.emplace_back(.5f, 1.f);
	data._asymmetryEnvelope._initial = 1.f;

	constexpr auto amplitude = .1f;
	TestVoice voice{ data, amplitude };
	auto sample = voice.renderSample();
	CHECK(sample == amplitude);
	for (int i = 1; i < 50; ++i)
	{
		const auto nextSample = voice.renderSample();
		CHECK(nextSample > 0.f);
		CHECK(sample > nextSample);
		sample = nextSample;
	}
	voice._renderer->restart();
	sample = voice.renderSample();
	CHECK(sample == amplitude);
	for (int i = 1; i < 100; ++i)
	{
		const auto nextSample = voice.renderSample();
		CHECK(nextSample > -amplitude);
		CHECK(sample > nextSample);
		sample = nextSample;
	}
	CHECK(voice.renderSample() == amplitude);
}

TEST_CASE("wave_square")
{
	aulos::VoiceData data;
	data._amplitudeEnvelope._initial = 1.f;
	data._amplitudeEnvelope._changes.emplace_back(.5f, 1.f);
	data._oscillationEnvelope._initial = 0.f;

	constexpr auto amplitude = .2f;
	TestVoice voice{ data, amplitude };
	for (int i = 0; i < 50; ++i)
		CHECK(voice.renderSample() == amplitude);
	for (int i = 0; i < 50; ++i)
		CHECK(voice.renderSample() == -amplitude);
	CHECK(voice.renderSample() == amplitude);
}

TEST_CASE("wave_triangle")
{
	aulos::VoiceData data;
	data._amplitudeEnvelope._initial = 1.f;
	data._amplitudeEnvelope._changes.emplace_back(.5f, 1.f);

	constexpr auto amplitude = .3f;
	TestVoice voice{ data, amplitude };
	auto sample = voice.renderSample();
	CHECK(sample == amplitude);
	for (int i = 1; i < 50; ++i)
	{
		const auto nextSample = voice.renderSample();
		CHECK(nextSample > -amplitude);
		CHECK(sample > nextSample);
		sample = nextSample;
	}
	sample = voice.renderSample();
	CHECK(sample == -amplitude);
	for (int i = 1; i < 25; ++i)
	{
		const auto nextSample = voice.renderSample();
		CHECK(nextSample < 0.f);
		CHECK(sample < nextSample);
		sample = nextSample;
	}
	voice._renderer->restart();
	sample = voice.renderSample();
	CHECK(sample == -amplitude);
	for (int i = 1; i < 50; ++i)
	{
		const auto nextSample = voice.renderSample();
		CHECK(nextSample < amplitude);
		CHECK(sample < nextSample);
		sample = nextSample;
	}
	CHECK(voice.renderSample() == amplitude);
}

TEST_CASE("wave_triangle_asymmetric")
{
	aulos::VoiceData data;
	data._amplitudeEnvelope._initial = 1.f;
	data._amplitudeEnvelope._changes.emplace_back(.5f, 1.f);
	data._asymmetryEnvelope._initial = .5f;

	constexpr auto amplitude = .4f;
	TestVoice voice{ data, amplitude };
	auto sample = voice.renderSample();
	CHECK(sample == amplitude);
	for (int i = 1; i < 50; ++i)
	{
		const auto nextSample = voice.renderSample();
		CHECK(nextSample > -amplitude / 3);
		CHECK(sample > nextSample);
		sample = nextSample;
	}
	voice._renderer->restart();
	sample = voice.renderSample();
	CHECK(sample == amplitude);
	for (int i = 1; i < 75; ++i)
	{
		const auto nextSample = voice.renderSample();
		CHECK(nextSample > -amplitude);
		CHECK(sample > nextSample);
		sample = nextSample;
	}
	sample = voice.renderSample();
	CHECK(sample == -amplitude);
	for (int i = 1; i < 25; ++i)
	{
		const auto nextSample = voice.renderSample();
		CHECK(nextSample < amplitude);
		CHECK(sample < nextSample);
		sample = nextSample;
	}
	CHECK(voice.renderSample() == amplitude);
}
