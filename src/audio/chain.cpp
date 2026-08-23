/**
 * @file
 * @brief What is done to samples between the buffer and the output.
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

#include <audio/chain.hpp>

#include "tuning.hpp"

#include <algorithm>

namespace wiola::audio {

Chain::Chain(StreamSpec spec, BandLayout layout)
    : layout_{layout}
{
    configure(spec);
}

void Chain::configure(StreamSpec spec)
{
    spec_ = spec;
    num_bands_ = 0;

    const units::Frequency highest{spec.sample_rate * tuning::highest_band_share};

    // The series ascends, so the first center that does not fit ends it.
    while (num_bands_ < layout_.count && layout_.center(num_bands_) <= highest)
        ++num_bands_;
}

StreamSpec Chain::spec() const noexcept
{
    return spec_;
}

std::size_t Chain::num_bands() const noexcept
{
    return num_bands_;
}

units::Frequency Chain::band_center(std::size_t index) const noexcept
{
    return layout_.center(index);
}

void Chain::process(std::span<float> samples) noexcept
{
    const float gain{volume_.load(std::memory_order_relaxed)};

    // Not for speed: at full volume the decoded samples pass through untouched.
    if (gain == 1.0F)
        return;

    for (float& sample : samples)
        sample *= gain;
}

void Chain::set_volume(float gain) noexcept
{
    // A bound rather than a clamp, so that a value that is not a number falls to silence.
    const float bounded{gain >= 0.0F ? std::min(gain, 1.0F) : 0.0F};

    volume_.store(bounded, std::memory_order_relaxed);
}

float Chain::volume() const noexcept
{
    return volume_.load(std::memory_order_relaxed);
}

} // namespace wiola::audio
