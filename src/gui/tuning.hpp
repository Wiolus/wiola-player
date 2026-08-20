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

/// Largest value of the position slider. A track maps onto it as a fraction of its whole length,
/// so this decides how finely any track can be scrubbed, short ones included.
inline constexpr int slider_range{1000};

} // namespace wiola::gui::tuning
