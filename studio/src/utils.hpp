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

class QString;

namespace aulos
{
	struct SequenceData;
}

constexpr auto kStepWidth = 15.0;
constexpr auto kTrackHeight = 40.0;
constexpr auto kTimelineHeight = 0.5 * kTrackHeight;
constexpr auto kAddTimeItemWidth = kStepWidth;
constexpr auto kAddTimeExtraWidth = kStepWidth;
constexpr auto kAddVoiceItemHeight = kTrackHeight * 0.75;
constexpr auto kMinVoiceItemWidth = kTrackHeight;

inline const QColor kHoverPenColor{ "#07f" };
inline const QColor kHoverBrushColor{ "#cff" };
inline const QColor kPressBrushColor{ "#8cf" };

struct VoiceColors
{
	QColor _brush;
	QColor _pen;
};

inline const std::array<VoiceColors, 2> kVoiceColors{
	VoiceColors{ "#fff", "#000" },
	VoiceColors{ "#ddd", "#000" },
};

QString makeSequenceName(const aulos::SequenceData&, bool rich = false);
