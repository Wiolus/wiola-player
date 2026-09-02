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
#include <memory>

namespace wiola::audio {

void Chain::configure(StreamSpec spec)
{
    for (const std::unique_ptr<Stage>& stage : stages_)
        stage->configure(spec);
}

void Chain::process(std::span<float> samples) noexcept
{
    bool lifted{false};

    for (const std::unique_ptr<Stage>& stage : stages_) {
        stage->process(samples);
        lifted = lifted || stage->lifted();
    }

    // Only a lift can reach past what an output takes, and no step knows until it has run.
    if (!lifted)
        return;

    for (float& sample : samples)
        sample = std::clamp(sample, -1.0F, 1.0F);
}

} // namespace wiola::audio
