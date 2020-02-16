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

#include <array>

#include <QColor>

extern const QColor kBackgroundColor;
extern const QColor kCursorColor;
extern const QColor kHoverBrushColor;
extern const QColor kHoverPenColor;
extern const QColor kPartBorderColor;
extern const QColor kPressBrushColor;

struct Colors
{
	QColor _brush;
	QColor _pen;
};

extern const std::array<Colors, 6> kFragmentColors;
extern const std::array<Colors, 2> kTimelineColors;
extern const std::array<Colors, 2> kVoiceColors;

struct TrackColors
{
	std::array<QColor, 2> _colors;
};

extern const std::array<TrackColors, 2> kTrackColors;

extern const std::array<QColor, 2> kPianorollBackgroundColor;
extern const QColor kPianorollGridColor;
extern const QColor kPianorollOctaveBorderColor;
extern const QColor kSoundBackgroundColor;
extern const QColor kSoundBorderColor;
