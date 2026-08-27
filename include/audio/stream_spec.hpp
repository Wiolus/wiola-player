/**
 * @file
 * @brief Description of an interleaved PCM stream.
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

#include <cstddef>

namespace wiola::audio {

/// A count of frames. Samples are counted plainly, being the length of a span; a frame is what
/// gets mistaken for one.
class Frames {
public:
    constexpr Frames() noexcept = default;

    constexpr explicit Frames(std::size_t count) noexcept
        : count_{count}
    {
    }

    [[nodiscard]] constexpr std::size_t count() const noexcept { return count_; }

    constexpr Frames& operator+=(Frames other) noexcept
    {
        count_ += other.count_;
        return *this;
    }

    friend constexpr auto operator<=>(Frames, Frames) = default;

    friend constexpr Frames operator+(Frames a, Frames b) noexcept
    {
        return Frames{a.count_ + b.count_};
    }

    friend constexpr Frames operator-(Frames a, Frames b) noexcept
    {
        return Frames{a.count_ - b.count_};
    }

private:
    std::size_t count_{0};
};

/**
 * Interleaved PCM, uncompressed: one measurement of one channel is a sample, one sample per
 * channel at the same instant is a frame, and `sample_rate` counts frames per second.
 *
 *     stereo, 4 frames:
 *
 *     frame 0     frame 1     frame 2     frame 3
 *     [ L  R ]    [ L  R ]    [ L  R ]    [ L  R ]
 *       ^  ^
 *       2 samples = 1 frame
 *
 * The other layout is planar: one array per channel, [ L L L L ] [ R R R R ]. Codecs commonly
 * decode that way. Everything here is interleaved, which is what a device callback takes.
 */
struct StreamSpec {
    units::Frequency sample_rate{48000.0};
    std::size_t num_channels{2};

    [[nodiscard]] constexpr bool valid() const noexcept
    {
        return sample_rate > units::Frequency{} && num_channels > 0;
    }

    [[nodiscard]] constexpr std::size_t samples_per(Frames num_frames) const noexcept
    {
        return num_frames.count() * num_channels;
    }

    [[nodiscard]] constexpr Frames frames_per(std::size_t num_samples) const noexcept
    {
        return Frames{num_samples / num_channels};
    }

    [[nodiscard]] constexpr std::size_t samples_per(units::Time duration) const noexcept
    {
        return samples_per(Frames{static_cast<std::size_t>(duration * sample_rate)});
    }

    friend constexpr bool operator==(const StreamSpec&, const StreamSpec&) = default;
};

} // namespace wiola::audio
