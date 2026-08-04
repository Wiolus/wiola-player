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
 * Turns a file into float frames.
 *
 * Samples are interleaved - one per channel per frame, in channel order - which is the layout a
 * device callback takes. A format that decodes its channels separately interleaves them on the
 * way out, so nothing above this class ever meets another layout.
 *
 * Only the shape of the stream and the next frames are exposed. Nothing here knows about
 * playback; a decoder is equally usable for transcoding or analysis.
 *
 * A subclass supplies one function, `decode`, and receives the counting, the clamping and the
 * end-of-stream question already answered.
 */
class Decoder {
public:
    Decoder(const Decoder&) = delete;
    Decoder& operator=(const Decoder&) = delete;
    Decoder(Decoder&&) = delete;
    Decoder& operator=(Decoder&&) = delete;
    virtual ~Decoder() = default;

    /// Fills whole frames and returns how many samples were written. Short means end of file.
    std::size_t render(std::span<float> output);

    [[nodiscard]] audio::StreamSpec spec() const noexcept { return spec_; }

    /// Total frames in the stream, and how many are still unread.
    [[nodiscard]] std::size_t num_frames() const noexcept { return num_frames_; }

    [[nodiscard]] std::size_t num_frames_left() const noexcept
    {
        return num_frames_ - num_frames_read_;
    }

    [[nodiscard]] bool exhausted() const noexcept { return num_frames_left() == 0; }

    /// Moves so that the next render starts at `frame_index`, counted from the beginning of the
    /// stream. Seeking to `num_frames()` leaves nothing to read. False when the stream will not
    /// move, in which case the position is where it was.
    bool seek(std::size_t frame_index);

protected:
    Decoder(audio::StreamSpec spec, std::size_t num_frames) noexcept
        : spec_{spec}
        , num_frames_{num_frames}
    {
    }

    /// Writes at most `num_frames` whole frames into `output`, and returns how many it wrote.
    /// Never asked for more frames than remain, nor for more than `output` can hold.
    virtual std::size_t decode(std::span<float> output, std::size_t num_frames) = 0;

    /// Moves the stream itself to `frame_index`. Never asked for a frame past the end.
    virtual bool seek_frame(std::size_t frame_index) = 0;

private:
    audio::StreamSpec spec_;
    std::size_t num_frames_;
    std::size_t num_frames_read_{0};
};

} // namespace wiola::codec
