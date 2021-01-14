// This file is part of the Aulos toolkit.
// Copyright (C) Sergei Blagodarin.
// SPDX-License-Identifier: Apache-2.0

#include <aulos/format.hpp>

#include <doctest.h>

namespace
{
	constexpr aulos::AudioFormat kMono44KHz{ 44'100, aulos::ChannelLayout::Mono };
	constexpr aulos::AudioFormat kStereo48KHz{ 48'000, aulos::ChannelLayout::Stereo };
}

TEST_CASE("format_construction")
{
	CHECK(kMono44KHz.bytesPerFrame() == 4);
	CHECK(kMono44KHz.channelCount() == 1);
	CHECK(kMono44KHz.channelLayout() == aulos::ChannelLayout::Mono);
	CHECK(kMono44KHz.samplingRate() == 44'100);

	CHECK(kStereo48KHz.bytesPerFrame() == 8);
	CHECK(kStereo48KHz.channelCount() == 2);
	CHECK(kStereo48KHz.channelLayout() == aulos::ChannelLayout::Stereo);
	CHECK(kStereo48KHz.samplingRate() == 48'000);
}

TEST_CASE("format_comparison")
{
	CHECK(kMono44KHz == aulos::AudioFormat(44'100, aulos::ChannelLayout::Mono));
	CHECK(kMono44KHz != aulos::AudioFormat(44'100, aulos::ChannelLayout::Stereo));
	CHECK(kMono44KHz != aulos::AudioFormat(48'000, aulos::ChannelLayout::Mono));
	CHECK(kMono44KHz != aulos::AudioFormat(48'000, aulos::ChannelLayout::Stereo));

	CHECK(kStereo48KHz != aulos::AudioFormat(44'100, aulos::ChannelLayout::Mono));
	CHECK(kStereo48KHz != aulos::AudioFormat(44'100, aulos::ChannelLayout::Stereo));
	CHECK(kStereo48KHz != aulos::AudioFormat(48'000, aulos::ChannelLayout::Mono));
	CHECK(kStereo48KHz == aulos::AudioFormat(48'000, aulos::ChannelLayout::Stereo));
}
