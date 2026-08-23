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

#include <core/macros.hpp>

#include <atomic>
#include <span>

namespace wiola::audio {

/**
 * Shapes samples on their way to the output.
 *
 * `process` is called from the thread that feeds the device: it does not allocate, lock or block,
 * and the work it does per sample is the same whatever the samples are. Settings are written from
 * another thread and are heard from the next call onwards, never partway through one.
 *
 * A chain outlives the track it is shaping. What a listener asked for is a property of the output
 * rather than of a file, so nothing here is reset by loading another one.
 */
class Chain {
public:
    Chain() = default;

    NO_COPY_SEMANTIC(Chain);
    NO_MOVE_SEMANTIC(Chain);

    ~Chain() = default;

    /// Applies the current settings to `samples`, in place. Interleaved or not makes no difference
    /// to what is applied now.
    void process(std::span<float> samples) noexcept;

    /// How loud the output is: 0 is silence, 1 the samples as they arrived. Values outside that
    /// range are clamped, so no setting can make the output louder than its source.
    void set_volume(float gain) noexcept;

    [[nodiscard]] float volume() const noexcept;

private:
    std::atomic<float> volume_{1.0F};
};

static_assert(std::atomic<float>::is_always_lock_free,
    "a setting is read on the thread that feeds the device, which cannot wait for a lock");

} // namespace wiola::audio
