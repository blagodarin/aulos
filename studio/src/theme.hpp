// This file is part of the Aulos toolkit.
// Copyright (C) Sergei Blagodarin.
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <array>

#include <QColor>

// Composition.
constexpr auto kStepWidth = 15.0;
constexpr auto kTrackHeight = 40.0;
constexpr auto kTimelineHeight = 0.5 * kTrackHeight;
constexpr auto kTimelineMarkingsHeight = kTimelineHeight;
constexpr auto kTimelineFontSize = kTimelineHeight * 0.75;
constexpr auto kCompositionHeaderHeight = kTimelineMarkingsHeight + kTimelineHeight;
constexpr auto kAddVoiceItemHeight = kTrackHeight * 0.75;
constexpr auto kAddVoiceArrowHeight = kAddVoiceItemHeight * 0.25;
constexpr auto kMinVoiceItemWidth = kTrackHeight;
constexpr auto kFragmentArrowWidth = kStepWidth / 2;
constexpr auto kFragmentFontSize = kTrackHeight * 0.75;
constexpr auto kVoiceNameFontSize = kTrackHeight * 0.5;
constexpr auto kVoiceNameMargin = (kTrackHeight - kVoiceNameFontSize) / 2;
constexpr auto kCompositionPageSwitchMargin = 50;
constexpr auto kLoopItemOffset = 2.0;
constexpr auto kLoopItemHeight = 8.0;
constexpr auto kCompositionFooterHeight = kLoopItemOffset + kLoopItemHeight;

// Pianoroll.
constexpr auto kNoteHeight = 20.0;
constexpr auto kNoteWidth = 15.0;
constexpr auto kWhiteKeyWidth = 3 * kNoteHeight;
constexpr auto kBlackKeyWidth = 2 * kNoteHeight;
constexpr size_t kPianorollStride = 8;

extern const QColor kBackgroundColor;
extern const QColor kHoverBrushColor;
extern const QColor kHoverPenColor;
extern const QColor kPartBorderColor;
extern const QColor kPressBrushColor;

struct Colors
{
	QColor _brush;
	QColor _pen;
};

extern const Colors kCursorColors;
extern const std::array<Colors, 6> kFragmentColors;
extern const std::array<Colors, 6> kFragmentHighlightColors;
extern const Colors kLoopItemColors;
extern const std::array<Colors, 2> kTimelineColors;
extern const Colors kTimelineOffsetMarkColors;
extern const std::array<Colors, 2> kVoiceColors;
extern const std::array<Colors, 2> kVoiceHighlightColors;

struct TrackColors
{
	std::array<QColor, 2> _colors;
};

extern const std::array<TrackColors, 2> kTrackColors;

extern const std::array<QColor, 2> kPianorollBackgroundColor;
extern const QColor kPianorollCoarseGridColor;
extern const QColor kPianorollFineGridColor;
extern const QColor kSoundBackgroundColor;
extern const QColor kSoundBorderColor;
