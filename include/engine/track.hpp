/**
 * @file
 * @brief A track in a queue: where it is, and what is known about it.
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

#include <filesystem>
#include <string>

namespace wiola::engine {

/**
 * One track of a queue: the file it is, and whatever is known about it.
 *
 * What is known is filled in as it is found out, and a listener is shown a queue long before any
 * of it has been. So a track with nothing known is a track with a name and no more - the file's
 * own - rather than a row that has to wait for something.
 */
struct Track {
    /// Where the file is. The only thing known from the start, and the only thing needed to play
    /// it.
    std::filesystem::path path;

    /// What to call it: what its tags say, or the file's own name until they have been read.
    std::string title;

    std::string artist;
    std::string album;

    /// How long it runs, or nothing while that is not known. Some formats say so in a header and
    /// some only by being read through, which is not worth doing to fill in a column.
    units::Time duration;

    /// The file's own name, for a track nothing else is known about.
    [[nodiscard]] static Track of(std::filesystem::path path);
};

} // namespace wiola::engine
