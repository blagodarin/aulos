// This file is part of the Aulos toolkit.
// Copyright (C) Sergei Blagodarin.
// SPDX-License-Identifier: Apache-2.0

#include "theme.hpp"

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

const Colors kTimelineOffsetMarkColors{ Qt::transparent, "#ddd" };

const std::array<Colors, 2> kVoiceColors{
	Colors{ "#444", "#ddd" },
	Colors{ "#333", "#ddd" },
};

const std::array<Colors, 2> kVoiceHighlightColors{
	Colors{ "#444", "#eee" },
	Colors{ "#333", "#eee" },
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
