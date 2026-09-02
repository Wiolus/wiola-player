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
#include <cstddef>

namespace wiola::gui::tuning {

/// How often the window asks where playback has reached. Nothing shown moves between these, so
/// longer reads as a slider that steps rather than slides.
inline constexpr std::chrono::milliseconds engine_poll_interval{100};

/// How tall the seek bar is drawn. Enough to aim a mouse at, and no more: it is a readout, not
/// the point of the window.
/// How much larger the name of what is playing is drawn than everything else, in points.
inline constexpr int track_name_point_size_step{3};

/// How wide a button showing only an icon is drawn, and how wide the play button is: wider than
/// its neighbours, since it is the one that is pressed most.
inline constexpr int icon_button_width{44};
inline constexpr int play_button_width{64};

/// How many pixels a side a track's picture is drawn at in the queue. A row is a line of a list
/// rather than a card, so the picture is what fits in a line and no more.
inline constexpr int cover_size{20};

/// How many pictures are read while drawing one turn. The rest are read on the turns after, so
/// that scrolling a long queue never waits for a file.
inline constexpr std::size_t covers_read_per_turn{4};

/// How many pictures are kept before they are let go of. A queue longer than this is one whose
/// far end is not being looked at.
inline constexpr std::size_t covers_kept{512};

/// How much taller than its picture a row of the queue is drawn.
inline constexpr int row_padding{4};

/// How wide the columns of the queue that do not stretch are drawn.
inline constexpr int artist_column_width{120};
inline constexpr int album_column_width{120};
inline constexpr int length_column_width{60};

/// How strongly the row of the track being played is tinted, out of 255. Enough to find at a
/// glance, and far short of what a row a listener has picked out looks like.
inline constexpr int playing_row_tint{40};

/// How wide the window is drawn before it is resized: enough for every column of the queue, so
/// that a length is not something a listener has to go looking for.
inline constexpr int window_width{620};

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
