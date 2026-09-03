/**
 * @file
 * @brief What frames are pulled from.
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

#include <audio/stream_spec.hpp>
#include <core/macros.hpp>

#include <cstddef>
#include <span>

namespace wiola::audio {

/// Frames on demand, in one format, interleaved. How long they last is not part of this.
class Source {
public:
    NO_COPY_SEMANTIC(Source);
    NO_MOVE_SEMANTIC(Source);

    virtual ~Source() = default;

    /// Fills whole frames and returns how many samples were written. Short means no more to give.
    virtual std::size_t render(std::span<float> interleaved) = 0;

    /// The format the frames are in. Fixed for the source's lifetime.
    [[nodiscard]] virtual StreamSpec spec() const noexcept = 0;

protected:
    Source() = default;
};

} // namespace wiola::audio
