/**
 * @file
 * @brief One step of the shaping between a decoder and a device.
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

#include <core/macros.hpp>
#include <pcm/stream_spec.hpp>

#include <span>

namespace wiola::audio {

/**
 * One step of the shaping a chain does, run in the order the chain holds them.
 *
 * Shaping happens on the thread that plays, so `process` does what it does and nothing else: no
 * lock to wait on, no allocation, no reaching for a file. What a listener asks for arrives by
 * whatever means the step itself provides, which for the ones here is an atomic.
 */
class Stage {
public:
    Stage() = default;

    NO_COPY_SEMANTIC(Stage);
    NO_MOVE_SEMANTIC(Stage);

    virtual ~Stage() = default;

    /// Retunes for a stream of `spec`, which a new track may have changed. Not the playing
    /// thread's: a chain configures between tracks, never during one.
    virtual void configure(pcm::StreamSpec spec) = 0;

    /// Shapes `samples` in place.
    virtual void process(std::span<float> samples) noexcept = 0;
};

} // namespace wiola::audio
