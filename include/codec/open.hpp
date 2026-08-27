/**
 * @file
 * @brief Picks a reader for a file.
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

#include <filesystem>
#include <memory>

namespace wiola::codec {

/// How opening a file went.
enum class OpenResult {
    opened,

    /// Nothing could be read from it: it is missing, unreadable or empty.
    unreadable,

    /// Nothing here reads a file like this one.
    unsupported,

    /// A format claimed it by its signature, and its own reader refused it.
    damaged,
};

/// A decoder, or why there is none.
struct Opened {
    std::unique_ptr<Decoder> decoder;
    OpenResult result{OpenResult::opened};

    [[nodiscard]] explicit operator bool() const noexcept { return decoder != nullptr; }
};

/// Opens `path` with whichever reader accepts it.
///
/// Whether a file can be decoded is settled by handing it to a reader, so the extension only
/// decides which one is asked first. A misnamed file therefore still plays, at the cost of an
/// attempt.
Opened open_file(const std::filesystem::path& path);

} // namespace wiola::codec
