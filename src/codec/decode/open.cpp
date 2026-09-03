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

#include <codec/open.hpp>

#include "flac_reader.hpp"
#include "format.hpp"
#include "mp3_reader.hpp"
#include "wav_reader.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <string>

namespace wiola::codec {

namespace {

/// Every format this layer reads. Adding one is a line here and nothing else.
std::span<const Format> formats()
{
    static const std::array list{WavReader::format(), FlacReader::format(), Mp3Reader::format()};

    return list;
}

std::string lowercased_extension(const std::filesystem::path& path)
{
    std::string extension{path.extension().string()};
    std::ranges::transform(extension, extension.begin(),
        [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

    return extension;
}

} // namespace

Opened open_file(const std::filesystem::path& path)
{
    const FileHead head{FileHead::read(path)};

    if (head.empty())
        return Opened{.decoder = nullptr, .result = OpenResult::unreadable};

    const std::string extension{lowercased_extension(path)};
    bool claimed{false};

    // A file is a format if that format's reader accepts it. Everything known about the file only
    // decides who is asked first, which is why the weakest reason still gets its turn.
    for (const Match reason : reasons) {
        for (const Format& format : formats()) {
            if (match_of(format, head, extension) != reason)
                continue;

            claimed = claimed || reason == Match::signature;

            if (std::unique_ptr<Decoder> decoder{format.open(path)})
                return Opened{.decoder = std::move(decoder), .result = OpenResult::opened};
        }
    }

    // A file carrying a format's signature is that format, so a reader refusing it is damage
    // rather than a format nothing here reads.
    return Opened{.decoder = nullptr,
        .result = claimed ? OpenResult::damaged : OpenResult::unsupported};
}

} // namespace wiola::codec
