/**
 * @file
 * @brief Drives a decoder through a device until it ends.
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

#include <codec/decoder.hpp>

#include <cstddef>
#include <optional>

namespace wiola::engine {

/**
 * Plays `source` to its end, and reports how often the device ran dry along the way.
 *
 * Nothing when no device could be started. Blocks until everything the source produced has been
 * heard, so a stream that never ends never returns.
 */
[[nodiscard]] std::optional<std::size_t> play(codec::Decoder& source);

} // namespace wiola::engine
