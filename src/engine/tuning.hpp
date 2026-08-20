/**
 * @file
 * @brief Values that decide how the engine schedules its work.
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

#include <utils/units.hpp>

#include <chrono>
#include <cstddef>

namespace wiola::engine::tuning {

/// How long decoding waits before testing a condition it cannot be woken for: a full buffer, or
/// a device still playing out. Longer delays a seek by up to this much.
inline constexpr std::chrono::milliseconds decode_poll_interval{2};

/// How much audio is held ahead of the device, and so how long decoding may stall before the
/// output falls silent. A seek pays for it twice, discarding what is held and decoding it again.
inline constexpr units::Time buffer_duration{std::chrono::milliseconds{250}};

/// Samples - not frames - decoded at a time, so a stereo stream advances half this many frames.
/// Larger spreads the cost of a call; smaller notices a stop or a seek sooner.
inline constexpr std::size_t decode_chunk_samples{1024};

} // namespace wiola::engine::tuning
