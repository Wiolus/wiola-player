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

Chain::Chain(StreamSpec spec, BandLayout layout)
    : spec_{spec}
    , equalizer_{spec, layout}
{
}

void Chain::configure(StreamSpec spec)
{
    spec_ = spec;
    equalizer_.configure(spec);
}

StreamSpec Chain::spec() const noexcept
{
    return spec_;
}

void Chain::process(std::span<float> samples) noexcept
{
    equalizer_.process(samples);
    volume_.process(samples);

    // Only a lift can reach past what an output takes: a band's, or a volume asked for beyond
    // what arrived. Neither is known until both have run.
    if (!equalizer_.shaping() && volume_.gain() <= 1.0F)
        return;

    for (float& sample : samples)
        sample = std::clamp(sample, -1.0F, 1.0F);
}

} // namespace wiola::audio
