/**
 * @file
 * @brief What this layer knows about a format, apart from how to decode it.
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

#include "format.hpp"

#include <algorithm>
#include <fstream>

namespace wiola::codec {

FileHead FileHead::read(const std::filesystem::path& path)
{
    FileHead head;
    std::ifstream file{path, std::ios::binary};

    file.read(reinterpret_cast<char*>(head.bytes_.data()),
        static_cast<std::streamsize>(head.bytes_.size()));
    head.size_ = static_cast<std::size_t>(file.gcount());

    return head;
}

bool FileHead::carries(const Marker& marker) const noexcept
{
    if (marker.offset + marker.bytes.size() > size_)
        return false;

    return std::ranges::equal(marker.bytes,
        std::span{bytes_}.subspan(marker.offset, marker.bytes.size()),
        [](char expected, std::byte actual) {
            return static_cast<unsigned char>(expected) == std::to_integer<unsigned char>(actual);
        });
}

Match match_of(const Format& format, const FileHead& head, std::string_view extension)
{
    const auto carried = [&head](const Marker& marker) {
        return head.carries(marker);
    };

    if (!format.markers.empty() && std::ranges::all_of(format.markers, carried))
        return Match::signature;

    if (std::ranges::contains(format.extensions, extension))
        return Match::extension;

    // Nothing was found, but a format with no signature was never going to be found by looking.
    if (format.markers.empty())
        return Match::possible;

    return Match::unlikely;
}

} // namespace wiola::codec
