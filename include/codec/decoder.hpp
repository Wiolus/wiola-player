/**
 * @file
 * @brief Interface every audio file reader implements.
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

#include <audio/stream_spec.hpp>

#include <cstddef>
#include <span>

namespace wiola::codec {

/**
 * Turns a file into interleaved float frames.
 *
 * Implementations own whatever the format needs - a file handle, a parser, a decoder - and
 * expose only the two facts a producer requires: the shape of the stream, and the next frames.
 * Nothing here knows about playback; a decoder is equally usable for transcoding or analysis.
 */
class Decoder {
public:
    Decoder() = default;

    Decoder(const Decoder&) = delete;
    Decoder& operator=(const Decoder&) = delete;
    Decoder(Decoder&&) = delete;
    Decoder& operator=(Decoder&&) = delete;
    virtual ~Decoder() = default;

    /// Fills whole frames and returns how many samples were written. Short means end of file.
    virtual std::size_t render(std::span<float> interleaved) = 0;

    [[nodiscard]] virtual audio::StreamSpec spec() const noexcept = 0;
    [[nodiscard]] virtual std::size_t num_frames() const noexcept = 0;
    [[nodiscard]] virtual std::size_t num_frames_left() const noexcept = 0;

    [[nodiscard]] bool exhausted() const noexcept { return num_frames_left() == 0; }
};

} // namespace wiola::codec
