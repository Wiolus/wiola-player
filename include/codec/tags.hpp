/**
 * @file
 * @brief What a file says about the track it holds.
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

#include <cstddef>
#include <filesystem>
#include <string>
#include <vector>

namespace wiola::codec {

/**
 * What a file says about the track it holds.
 *
 * Every part of it is what a file claims, not what has been checked: a title is whatever bytes
 * were in the tag, and a picture is whatever bytes were where a picture goes. Nothing here has
 * been decoded, and nothing here can be trusted further than being shown.
 */
struct Tags {
    std::string title;
    std::string artist;
    std::string album;

    /// The picture as the file stored it, and the kind it claims to be. Left empty when there is
    /// none, or when what was there made no sense.
    std::vector<std::byte> art;
    std::string art_type;

    [[nodiscard]] bool empty() const noexcept
    {
        return title.empty() && artist.empty() && album.empty() && art.empty();
    }
};

/// Reads what `path` says about itself, without decoding any of it. Anything that cannot be made
/// sense of is left out rather than guessed at, so a file that says nothing, a file full of
/// nonsense and a file that is not there all answer the same way: with nothing.
[[nodiscard]] Tags read_tags(const std::filesystem::path& path);

} // namespace wiola::codec
