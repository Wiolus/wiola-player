/**
 * @file
 * @brief The file a decoding library reads through.
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
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <memory>

namespace wiola::codec {

/// Where a seek counts from.
enum class SeekFrom {
    start,
    current,
    end,
};

/**
 * Bytes of a file, handed to a decoding library one read at a time.
 *
 * Decoders are given this rather than a filename. A filename is not the same thing on every
 * platform - it is wide characters on Windows, and on Android an asset may have no path at all -
 * so no third-party call in this layer takes one, and none of them has to be told which platform
 * it is on.
 */
class FileStream final {
public:
    /// Opens `path` for reading. Null when it cannot be opened.
    [[nodiscard]] static std::unique_ptr<FileStream> open(const std::filesystem::path& path);

    /// Reads up to `num_bytes` into `out`, and returns how many bytes it got. Fewer than asked
    /// for means the end of the file.
    [[nodiscard]] std::size_t read(void* out, std::size_t num_bytes);

    /// Moves the cursor. False when the file will not go there.
    [[nodiscard]] bool seek(std::int64_t offset, SeekFrom from);

    /// Writes the cursor into `cursor`. False when it cannot be told.
    [[nodiscard]] bool tell(std::int64_t& cursor);

private:
    std::ifstream file_;
};

/**
 * The same stream in the shape a decoding library expects.
 *
 * Each library declares its own seek origin type and its own width of file offset, so those are
 * given here rather than assumed to agree between them.
 */
template<typename Origin, Origin from_start, Origin from_current, typename Cursor>
struct StreamCallbacks {
    static std::size_t read(void* user, void* out, std::size_t num_bytes)
    {
        return static_cast<FileStream*>(user)->read(out, num_bytes);
    }

    static unsigned int seek(void* user, int offset, Origin origin)
    {
        const SeekFrom from{origin == from_start ? SeekFrom::start
                : origin == from_current         ? SeekFrom::current
                                                 : SeekFrom::end};

        return static_cast<FileStream*>(user)->seek(offset, from) ? 1U : 0U;
    }

    static unsigned int tell(void* user, Cursor* cursor)
    {
        std::int64_t position{0};

        if (!static_cast<FileStream*>(user)->tell(position))
            return 0U;

        *cursor = static_cast<Cursor>(position);

        return 1U;
    }
};

} // namespace wiola::codec
