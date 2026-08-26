/**
 * @file
 * @brief How loud the output is.
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

#include <audio/volume.hpp>

#include <algorithm>

namespace wiola::audio {

void Volume::set_gain(float gain) noexcept
{
    // A bound rather than a clamp, so that a value that is not a number falls to silence.
    const float bounded{gain >= 0.0F ? std::min(gain, 1.0F) : 0.0F};

    gain_.store(bounded, std::memory_order_relaxed);
}

float Volume::gain() const noexcept
{
    return gain_.load(std::memory_order_relaxed);
}

void Volume::process(std::span<float> samples) noexcept
{
    const float gain{gain_.load(std::memory_order_relaxed)};

    // Not for speed: at full volume the decoded samples pass through untouched.
    if (gain == 1.0F)
        return;

    for (float& sample : samples)
        sample *= gain;
}

} // namespace wiola::audio
