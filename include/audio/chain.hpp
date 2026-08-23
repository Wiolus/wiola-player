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

#include <audio/stream_spec.hpp>
#include <core/macros.hpp>
#include <utils/units.hpp>

#include <atomic>
#include <cstddef>
#include <span>

namespace wiola::audio {

/**
 * Where a chain's bands sit: `count` centers from `first`, each `ratio` times the one before it.
 *
 * A ratio of two is one band per octave, which is the default: ten of them from 31.25 Hz.
 */
struct BandLayout {
    units::Frequency first{31.25};
    std::size_t count{10};
    double ratio{2.0};

    /// Where band `index` sits. Indices past `count` continue the series.
    [[nodiscard]] constexpr units::Frequency center(std::size_t index) const noexcept
    {
        units::Frequency value{first};

        for (std::size_t i = 0; i < index; ++i)
            value = value * ratio;

        return value;
    }
};

/**
 * Shapes samples on their way to the output.
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

    ~Chain() = default;

    /// Takes the format the samples are in. Settings are kept.
    void configure(StreamSpec spec);

    [[nodiscard]] StreamSpec spec() const noexcept;

    /// How many bands the format can carry, counting up from the lowest. At most `count`: a band
    /// too close to half the sample rate cannot be given its width, and is left out.
    [[nodiscard]] std::size_t num_bands() const noexcept;

    /// Where band `index` sits. Indices at `num_bands()` and beyond have no band.
    [[nodiscard]] units::Frequency band_center(std::size_t index) const noexcept;

    /// Applies the current settings to `samples`, in place.
    void process(std::span<float> samples) noexcept;

    /// How loud the output is: 0 is silence, 1 the samples as they arrived. Clamped to that
    /// range, so nothing here amplifies.
    void set_volume(float gain) noexcept;

    [[nodiscard]] float volume() const noexcept;

private:
    StreamSpec spec_{};
    BandLayout layout_{};
    std::size_t num_bands_{0};
    std::atomic<float> volume_{1.0F};
};

static_assert(std::atomic<float>::is_always_lock_free,
    "a setting is read on the thread that feeds the device, which cannot wait for a lock");

} // namespace wiola::audio
