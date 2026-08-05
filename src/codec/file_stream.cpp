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

#include "file_stream.hpp"

namespace wiola::codec {

std::unique_ptr<FileStream> FileStream::open(const std::filesystem::path& path)
{
    auto stream = std::make_unique<FileStream>();
    stream->file_.open(path, std::ios::binary);

    if (!stream->file_)
        return nullptr;

    return stream;
}

std::size_t FileStream::read(void* out, std::size_t num_bytes)
{
    file_.read(static_cast<char*>(out), static_cast<std::streamsize>(num_bytes));

    return static_cast<std::size_t>(file_.gcount());
}

bool FileStream::seek(std::int64_t offset, SeekFrom from)
{
    // Reading to the end leaves the stream in a failed state, and a decoder is entitled to seek
    // back out of it.
    file_.clear();

    const std::ios::seekdir direction{from == SeekFrom::start ? std::ios::beg
            : from == SeekFrom::current                       ? std::ios::cur
                                                              : std::ios::end};

    file_.seekg(static_cast<std::streamoff>(offset), direction);

    return static_cast<bool>(file_);
}

bool FileStream::tell(std::int64_t& cursor)
{
    file_.clear();

    const std::streampos position{file_.tellg()};

    if (position < 0)
        return false;

    cursor = static_cast<std::int64_t>(position);

    return true;
}

} // namespace wiola::codec
