// This file is part of the Aulos toolkit.
// Copyright (C) Sergei Blagodarin.
// SPDX-License-Identifier: Apache-2.0

#include <aulos/composition.hpp>
#include <aulos/data.hpp>
#include <aulos/format.hpp>
#include <aulos/renderer.hpp>

#include <doctest.h>

#include <array>
#include <functional>

using namespace std::chrono_literals;

namespace
{
	enum class Notes
	{
		No,
		Yes,
	};

	enum class Loop
	{
		No,
		Yes,
	};

	enum class Looping
	{
		No,
		Yes,
	};

	auto makeTestRenderer(Notes notes, Loop loop, Looping looping)
	{
		aulos::CompositionData composition;
		const auto voice = std::make_shared<aulos::VoiceData>();
		voice->_amplitudeEnvelope._changes.emplace_back(0ms, 1.f);
		voice->_amplitudeEnvelope._changes.emplace_back(1001ms, 1.f);
		voice->_asymmetryEnvelope._changes.emplace_back(0ms, 1.f);
		const auto& part = composition._parts.emplace_back(std::make_shared<aulos::PartData>(voice));
		const auto& track = part->_tracks.emplace_back(std::make_shared<aulos::TrackData>(std::make_shared<aulos::TrackProperties>()));
		const auto& sequence = track->_sequences.emplace_back(std::make_shared<aulos::SequenceData>());
		if (notes == Notes::Yes)
		{
			sequence->_sounds.emplace_back(0u, aulos::Note::A4);
			sequence->_sounds.emplace_back(1u, aulos::Note::A4);
		}
		track->_fragments.emplace(0u, sequence);
		if (loop == Loop::Yes)
		{
			composition._loopOffset = 1;
			composition._loopLength = 1;
		}
		return aulos::Renderer::create(*composition.pack(), { 8'000, aulos::ChannelLayout::Mono }, looping == Looping::Yes);
	}

	size_t renderAction(aulos::Renderer& renderer, size_t frames)
	{
		static std::array<float, 24'001> buffer;
		return renderer.render(buffer.data(), frames);
	}

	size_t skipAction(aulos::Renderer& renderer, size_t frames)
	{
		return renderer.skipFrames(frames);
	}

	void expectEmpty(Notes notes, Loop loop, Looping looping, const std::function<size_t(aulos::Renderer&, size_t)>& action)
	{
		const auto renderer = ::makeTestRenderer(notes, loop, looping);
		CHECK(renderer->loopOffset() == 0);
		CHECK(renderer->currentOffset() == 0);
		CHECK(action(*renderer, 1) == 0);
		CHECK(renderer->currentOffset() == 0);
	}

	void expectLoop(Notes notes, Loop loop, Looping looping, const std::function<size_t(aulos::Renderer&, size_t)>& action, size_t loopStart, size_t loopEnd)
	{
		const auto renderer = ::makeTestRenderer(notes, loop, looping);
		CHECK(renderer->loopOffset() == loopStart);
		CHECK(renderer->currentOffset() == 0);
		CHECK(action(*renderer, loopEnd - 2) == loopEnd - 2);
		CHECK(renderer->currentOffset() == loopEnd - 2);
		CHECK(action(*renderer, 1) == 1);
		CHECK(renderer->currentOffset() == loopEnd - 1);
		CHECK(action(*renderer, 1) == 1);
		CHECK(renderer->currentOffset() == loopStart);
		CHECK(action(*renderer, 1) == 1);
		CHECK(renderer->currentOffset() == loopStart + 1);
		CHECK(action(*renderer, loopEnd - loopStart + 1) == loopEnd - loopStart + 1);
		CHECK(renderer->currentOffset() == loopStart + 2);
	}

	void expectNoLoop(Notes notes, Loop loop, Looping looping, const std::function<size_t(aulos::Renderer&, size_t)>& action)
	{
		const auto renderer = ::makeTestRenderer(notes, loop, looping);
		CHECK(renderer->loopOffset() == 0);
		CHECK(renderer->currentOffset() == 0);
		CHECK(action(*renderer, 16'007) == 16'007);
		CHECK(renderer->currentOffset() == 16'007);
		CHECK(action(*renderer, 1) == 1);
		CHECK(renderer->currentOffset() == 16'008);
		CHECK(action(*renderer, 1) == 0);
		CHECK(renderer->currentOffset() == 16'008);
	}
}

TEST_CASE("Render (no notes, no loop, no looping)")
{
	expectEmpty(Notes::No, Loop::No, Looping::No, renderAction);
}

TEST_CASE("Render (with notes, no loop, no looping)")
{
	expectNoLoop(Notes::Yes, Loop::No, Looping::No, renderAction);
}

TEST_CASE("Render (no notes, with loop, no looping)")
{
	expectEmpty(Notes::No, Loop::Yes, Looping::No, renderAction);
}

TEST_CASE("Render (with notes, with loop, no looping)")
{
	expectNoLoop(Notes::Yes, Loop::Yes, Looping::No, renderAction);
}

TEST_CASE("Render (no notes, no loop, with looping)")
{
	expectLoop(Notes::No, Loop::No, Looping::Yes, renderAction, 0, 8'000);
}

TEST_CASE("Render (with notes, no loop, with looping)")
{
	expectLoop(Notes::Yes, Loop::No, Looping::Yes, renderAction, 0, 24'000);
}

TEST_CASE("Render (no notes, with loop, with looping)")
{
	expectLoop(Notes::No, Loop::Yes, Looping::Yes, renderAction, 8'000, 16'000);
}

TEST_CASE("Render (with notes, with loop, with looping)")
{
	expectLoop(Notes::Yes, Loop::Yes, Looping::Yes, renderAction, 8'000, 16'000);
}

TEST_CASE("Skip (no notes, no loop, no looping)")
{
	expectEmpty(Notes::No, Loop::No, Looping::No, skipAction);
}

TEST_CASE("Skip (with notes, no loop, no looping)")
{
	expectNoLoop(Notes::Yes, Loop::No, Looping::No, skipAction);
}

TEST_CASE("Skip (no notes, with loop, no looping)")
{
	expectEmpty(Notes::No, Loop::Yes, Looping::No, skipAction);
}

TEST_CASE("Skip (with notes, with loop, no looping)")
{
	expectNoLoop(Notes::Yes, Loop::Yes, Looping::No, skipAction);
}

TEST_CASE("Skip (no notes, no loop, with looping)")
{
	expectLoop(Notes::No, Loop::No, Looping::Yes, skipAction, 0, 8'000);
}

TEST_CASE("Skip (with notes, no loop, with looping)")
{
	expectLoop(Notes::Yes, Loop::No, Looping::Yes, skipAction, 0, 24'000);
}

TEST_CASE("Skip (no notes, with loop, with looping)")
{
	expectLoop(Notes::No, Loop::Yes, Looping::Yes, skipAction, 8'000, 16'000);
}

TEST_CASE("Skip (with notes, with loop, with looping)")
{
	expectLoop(Notes::Yes, Loop::Yes, Looping::Yes, skipAction, 8'000, 16'000);
}
