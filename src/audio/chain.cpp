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

#include <algorithm>

namespace wiola::audio {

void Chain::process(std::span<float> samples) noexcept
{
    const float gain{volume_.load(std::memory_order_relaxed)};

    // Not an optimization of the multiply, which is nothing. It is what keeps the samples the
    // decoder produced from being touched at all while no one is asking for a change.
    if (gain == 1.0F)
        return;

    for (float& sample : samples)
        sample *= gain;
}

void Chain::set_volume(float gain) noexcept
{
    // Written as a bound on the low end rather than a clamp, so that a value that is not a number
    // is silence rather than whatever a comparison against it happens to answer.
    const float bounded{gain >= 0.0F ? std::min(gain, 1.0F) : 0.0F};

    volume_.store(bounded, std::memory_order_relaxed);
}

float Chain::volume() const noexcept
{
    return volume_.load(std::memory_order_relaxed);
}

} // namespace wiola::audio
