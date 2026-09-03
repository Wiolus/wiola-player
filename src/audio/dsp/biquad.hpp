/**
 * @file
 * @brief One second-order section.
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

#include <pcm/stream_spec.hpp>
#include <utils/units.hpp>

#include <cstddef>
#include <span>
#include <vector>

namespace wiola::audio {

/// What a second-order section multiplies by, divided through so that the leading denominator
/// term is one. The default is the section that leaves every sample as it found it.
struct BiquadCoefficients {
    float b0{1.0F};
    float b1{0.0F};
    float b2{0.0F};
    float a1{0.0F};
    float a2{0.0F};

    /// A section that lifts or cuts `gain_db` around `center`, reaching less far to either side
    /// as `quality` rises. A rate or a quality that means nothing gives the section that does
    /// nothing.
    [[nodiscard]] static BiquadCoefficients peaking(pcm::StreamSpec spec, units::Frequency center,
        double quality, double gain_db) noexcept;
};

/**
 * One second-order section, run over interleaved frames.
 *
 * `process` is called from the thread that feeds the device: it does not allocate, lock or block.
 * What it carries between calls is a pair of numbers per channel, so a block boundary is not
 * something a listener can hear the sound cross.
 */
class Biquad {
public:
    /// Takes what it multiplies by and how many channels it runs over. Forgets what it was
    /// carrying, so this is not for a stream already playing.
    void configure(BiquadCoefficients coefficients, std::size_t num_channels);

    /// Takes new coefficients and keeps what it is carrying, so a gain moved during a track is
    /// heard as the sound changing rather than as a click.
    void retune(BiquadCoefficients coefficients) noexcept;

    /// Shapes whole frames of `samples` in place.
    void process(std::span<float> samples) noexcept;

private:
    BiquadCoefficients coefficients_;

    /// Two numbers per channel, which is what a transposed direct form II carries.
    std::vector<float> carried_;
    std::size_t num_channels_{0};
};

} // namespace wiola::audio
