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
 */
struct StreamSpec {
    units::Frequency sample_rate{48000.0};
    std::size_t num_channels{2};

    [[nodiscard]] constexpr bool valid() const noexcept
    {
        return sample_rate > units::Frequency{} && num_channels > 0;
    }

    [[nodiscard]] constexpr std::size_t samples_per(std::size_t num_frames) const noexcept
    {
        return num_frames * num_channels;
    }

    [[nodiscard]] constexpr std::size_t frames_per(std::size_t num_samples) const noexcept
    {
        return num_samples / num_channels;
    }

    [[nodiscard]] constexpr std::size_t samples_per(units::Time duration) const noexcept
    {
        return samples_per(static_cast<std::size_t>(duration * sample_rate));
    }

    friend constexpr bool operator==(const StreamSpec&, const StreamSpec&) = default;
};

} // namespace wiola::audio
