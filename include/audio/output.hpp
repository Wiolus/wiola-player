/**
 * @file
 * @brief Where playback runs.
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

namespace wiola::audio {

/// Somewhere frames are played, which is started and stopped and says how much has been heard.
class Output {
public:
    NO_COPY_SEMANTIC(Output);
    NO_MOVE_SEMANTIC(Output);

    virtual ~Output() = default;

    /// Begins playing what the output pulls. False when there is none to be had.
    [[nodiscard]] virtual bool start() noexcept = 0;

    /// Halts playing. Starting again is the caller's to ask for.
    virtual void stop() noexcept = 0;

    [[nodiscard]] virtual bool running() const noexcept = 0;

    /// Frames played since the last reset, which is what has been heard.
    [[nodiscard]] virtual Frames frames_played() const noexcept = 0;

protected:
    Output() = default;
};

} // namespace wiola::audio
