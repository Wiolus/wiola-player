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

#pragma once

#include <audio/equalizer.hpp>
#include <audio/stream_spec.hpp>
#include <audio/volume.hpp>
#include <core/macros.hpp>

#include <span>

namespace wiola::audio {

/**
 * What every sample passes through on its way to the output, in order.
 *
 * `process` is called from the thread that feeds the device: it does not allocate, lock or block.
 * Settings are written from another thread and take effect on the next call, never partway
 * through one.
 *
 * A chain belongs to the output rather than to a track, so loading one resets nothing.
 */
class Chain {
public:
    explicit Chain(StreamSpec spec, BandLayout layout = {});

    NO_COPY_SEMANTIC(Chain);
    NO_MOVE_SEMANTIC(Chain);

    /// Takes the format the samples are in. Settings are kept. Not to be called while the output
    /// is running: it rebuilds what `process` reads.
    void configure(StreamSpec spec);

    [[nodiscard]] StreamSpec spec() const noexcept;

    /// Applies every stage to whole frames of `samples`, in place.
    void process(std::span<float> samples) noexcept;

    [[nodiscard]] Volume& volume() noexcept { return volume_; }

    [[nodiscard]] const Volume& volume() const noexcept { return volume_; }

    [[nodiscard]] Equalizer& equalizer() noexcept { return equalizer_; }

    [[nodiscard]] const Equalizer& equalizer() const noexcept { return equalizer_; }

private:
    StreamSpec spec_{};
    Volume volume_;
    Equalizer equalizer_;
};

} // namespace wiola::audio
