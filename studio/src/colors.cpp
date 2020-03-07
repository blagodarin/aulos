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

#include "colors.hpp"

const QColor kBackgroundColor{ "#222" };
const QColor kHoverBrushColor{ "#999" };
const QColor kHoverPenColor{ "#ddd" };
const QColor kPartBorderColor{ "#999" };
const QColor kPressBrushColor{ "#888" };

extern const Colors kCursorColors{ "#fff", "#000" };

const std::array<Colors, 6> kFragmentColors{
	Colors{ "#e77", "#400" },
	Colors{ "#ee7", "#440" },
	Colors{ "#7e7", "#040" },
	Colors{ "#7ee", "#044" },
	Colors{ "#77e", "#004" },
	Colors{ "#e7e", "#404" },
};

const std::array<Colors, 6> kFragmentHighlightColors{
	Colors{ "#400", "#f00" },
	Colors{ "#440", "#ff0" },
	Colors{ "#040", "#0f0" },
	Colors{ "#044", "#0ff" },
	Colors{ "#004", "#00f" },
	Colors{ "#404", "#f0f" },
};

const std::array<Colors, 2> kTimelineColors{
	Colors{ "#444", "#ddd" },
	Colors{ "#333", "#ddd" },
};

const std::array<Colors, 2> kVoiceColors{
	Colors{ "#444", "#ddd" },
	Colors{ "#333", "#ddd" },
};

const std::array<TrackColors, 2> kTrackColors{
	TrackColors{ "#777", "#666" },
	TrackColors{ "#666", "#555" },
};

const std::array<QColor, 2> kPianorollBackgroundColor{ "#333", "#222" };
const QColor kPianorollCoarseGridColor{ "#666" };
const QColor kPianorollFineGridColor{ "#444" };
const QColor kSoundBackgroundColor{ "#ee0" };
const QColor kSoundBorderColor{ "#880" };
