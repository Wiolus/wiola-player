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
/// How much larger the name of what is playing is drawn than everything else, in points.
inline constexpr int track_name_point_size_step{3};

/// How wide the play button is drawn. Wider than the rest: it is the one that is pressed most.
inline constexpr int play_button_width{90};

/// How tall the queue is drawn before the window is resized. Enough to see a handful of tracks.
inline constexpr int playlist_height{120};

inline constexpr int seek_bar_height{12};

/// How wide the line marking the played edge of the seek bar is.
inline constexpr int seek_bar_edge_width{2};

/// How wide the volume slider is drawn.
inline constexpr int volume_slider_width{70};

/// Where the volume slider starts, out of a hundred.
inline constexpr int full_volume{100};

/// How wide the volume readout is drawn. Fixed, so a slider moving between "5%" and "140%" does
/// not shift the row.
inline constexpr int volume_value_width{36};

/// How wide the button that opens the volume past full is drawn.
inline constexpr int boost_button_width{24};

/// Where the volume slider ends once the listener has asked for more than arrived.
inline constexpr int boosted_volume{140};

/// How the slider's position becomes a gain. Above one because hearing is not linear: without it
/// the whole audible range would crowd into the bottom of the travel.
inline constexpr double volume_curve{2.0};

/// How tall a band slider is drawn. Enough travel to place a band by eye.
inline constexpr int band_slider_height{110};

/// How wide a band's column is drawn. Fixed, so a readout changing between "0.0" and "-12.0"
/// does not shift the row.
inline constexpr int band_column_width{42};

/// Steps a gain slider takes to the decibel, since a slider counts in whole numbers only.
inline constexpr int gain_steps_per_db{10};

/// How far a gain slider moves for a press of an arrow key, in its own steps.
inline constexpr int gain_step{5};

} // namespace wiola::gui::tuning
