/**
 * @file
 * @brief The ceiling an output takes.
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

#include <audio/dsp/stage.hpp>
#include <pcm/stream_spec.hpp>

#include <span>

namespace wiola::audio {

/// Holds every sample to what an output takes. Past full scale there is nothing to hear, so what
/// reaches past it is flattened rather than wrapped.
class Clip final : public Stage {
public:
    /// Nothing here follows the stream, so this is what it costs to say so.
    void configure(pcm::StreamSpec spec) noexcept override;

    void process(std::span<float> samples) noexcept override;
};

} // namespace wiola::audio
