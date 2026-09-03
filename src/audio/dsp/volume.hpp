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

#pragma once

#include <audio/dsp/stage.hpp>
#include <pcm/stream_spec.hpp>

#include <atomic>
#include <span>

namespace wiola::audio {

/// One gain over everything: 0 is silence, 1 the samples as they arrived, and up to
/// `max_volume_boost` more than arrived.
class Volume final : public Stage {
public:
    /// Clamped to that range, and anything that is not a number is taken as silence.
    void set_gain(float gain) noexcept;

    [[nodiscard]] float gain() const noexcept;

    /// Nothing here follows the stream, so this is what it costs to say so.
    void configure(pcm::StreamSpec spec) noexcept override;

    void process(std::span<float> samples) noexcept override;

private:
    std::atomic<float> gain_{1.0F};
};

static_assert(std::atomic<float>::is_always_lock_free,
    "a setting is read on the thread that feeds the device, which cannot wait for a lock");

} // namespace wiola::audio
