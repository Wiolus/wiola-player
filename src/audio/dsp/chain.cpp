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

#include <audio/dsp/chain.hpp>

namespace wiola::audio {

void Chain::add(Stage& stage)
{
    stages_.push_back(&stage);
}

void Chain::configure(pcm::StreamSpec spec)
{
    for (Stage* stage : stages_)
        stage->configure(spec);
}

void Chain::process(std::span<float> samples) noexcept
{
    for (Stage* stage : stages_)
        stage->process(samples);

    clip_.process(samples);
}

} // namespace wiola::audio
