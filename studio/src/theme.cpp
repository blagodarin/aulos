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

const std::array<Colors, 12> kFragmentColors{
	Colors{ "#c00", "#200" },
	Colors{ "#c60", "#210" },
	Colors{ "#cc0", "#220" },
	Colors{ "#6c0", "#120" },
	Colors{ "#0c0", "#020" },
	Colors{ "#0c6", "#021" },
	Colors{ "#0cc", "#022" },
	Colors{ "#06c", "#012" },
	Colors{ "#00c", "#002" },
	Colors{ "#60c", "#102" },
	Colors{ "#c0c", "#202" },
	Colors{ "#c06", "#201" },
};

const std::array<Colors, 12> kFragmentHighlightColors{
	Colors{ "#400", "#f55" },
	Colors{ "#420", "#fa5" },
	Colors{ "#440", "#ff5" },
	Colors{ "#240", "#af5" },
	Colors{ "#040", "#5f5" },
	Colors{ "#042", "#5fa" },
	Colors{ "#044", "#5ff" },
	Colors{ "#024", "#5af" },
	Colors{ "#004", "#55f" },
	Colors{ "#204", "#a5f" },
	Colors{ "#404", "#f5f" },
	Colors{ "#402", "#f5a" },
};

const Colors kLoopItemColors{ Qt::darkCyan, Qt::transparent };

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
