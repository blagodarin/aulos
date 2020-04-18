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

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include <aulos/data.hpp>

#include <doctest.h>

TEST_CASE("VoiceRenderer")
{
	aulos::VoiceData data;
	data._amplitudeEnvelope._initial = 1.f;
	data._amplitudeEnvelope._changes.emplace_back(.5f, 1.f);
	const auto renderer = aulos::VoiceRenderer::create(data, 44'000, 1);
	REQUIRE(renderer);
	CHECK(renderer->channels() == 1);
	CHECK(renderer->samplingRate() == 44'000);
	CHECK(renderer->totalSamples() == 22'000);
	renderer->start(aulos::Note::A4, 1.f); // A4 note frequency is exactly 440 Hz, so the period should be exactly 100 samples.
	const auto renderSample = [&renderer] {
		float sample = 0.f;
		REQUIRE(renderer->render(&sample, sizeof sample) == sizeof sample);
		return sample;
	};
	auto sample = renderSample();
	CHECK(sample == 1.f);
	for (int i = 1; i < 50; ++i)
	{
		const auto nextSample = renderSample();
		CHECK(nextSample < sample);
		sample = nextSample;
	}
	sample = renderSample();
	CHECK(sample == -1.f);
	for (int i = 1; i < 50; ++i)
	{
		const auto nextSample = renderSample();
		CHECK(nextSample > sample);
		sample = nextSample;
	}
}
