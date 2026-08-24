/**
 * @file
 * @brief Values that decide how the window follows playback.
 * @author Roman Glaz
 * @copyright © 2026, <vokerlee@gmail.com>
 *
 * Wiola is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Wiola is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Wiola. If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <chrono>

namespace wiola::gui::tuning {

/// How often the window asks where playback has reached. Nothing shown moves between these, so
/// longer reads as a slider that steps rather than slides.
inline constexpr std::chrono::milliseconds engine_poll_interval{100};

/// How tall the seek bar is drawn. Enough to aim a mouse at, and no more: it is a readout, not
/// the point of the window.
inline constexpr int seek_bar_height{12};

/// How wide the line marking the played edge of the seek bar is.
inline constexpr int seek_bar_edge_width{2};

/// How wide the volume slider is drawn.
inline constexpr int volume_slider_width{70};

/// Where the volume slider starts, out of a hundred.
inline constexpr int full_volume{100};

/// How the slider's position becomes a gain. Above one because hearing is not linear: without it
/// the whole audible range would crowd into the bottom of the travel.
inline constexpr double volume_curve{2.0};

} // namespace wiola::gui::tuning
