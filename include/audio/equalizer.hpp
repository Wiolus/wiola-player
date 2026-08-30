/**
 * @file
 * @brief Lift and cut, one band at a time.
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

#include <audio/stage.hpp>
#include <audio/stream_spec.hpp>
#include <audio/tuning.hpp>
#include <core/macros.hpp>
#include <utils/units.hpp>

#include <cstddef>
#include <memory>
#include <span>

namespace wiola::audio {

/**
 * Where the bands sit: `count` centers from `first`, each `ratio` times the one before it.
 *
 * A ratio of two is one band per octave, which is the default: ten of them from 31.25 Hz.
 */
struct BandLayout {
    units::Frequency first{tuning::first_band_center};
    std::size_t count{tuning::num_layout_bands};
    double ratio{tuning::band_ratio};
    double quality{tuning::band_quality};

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
 * Lifts and cuts bands of the spectrum, cutting beforehand to make room for what they add.
 *
 * `process` is called from the thread that feeds the device: it does not allocate, lock or block.
 * Settings are written from another thread and take effect on the next call, never partway
 * through one.
 */
class Equalizer final : public Stage {
public:
    explicit Equalizer(StreamSpec spec, BandLayout layout = {});

    NO_COPY_SEMANTIC(Equalizer);
    NO_MOVE_SEMANTIC(Equalizer);

    ~Equalizer();

    /// Takes the format the samples are in. Settings are kept. Not to be called while the output
    /// is running: it rebuilds what `process` reads.
    void configure(StreamSpec spec) override;

    /// Where the bands sit, whether or not the format can carry them all. A gain is kept for
    /// every one it names.
    [[nodiscard]] const BandLayout& layout() const noexcept;

    /// How many bands the format can carry, counting up from the lowest. At most `count`: a band
    /// too close to half the sample rate cannot be given its width, and is left out.
    [[nodiscard]] std::size_t num_bands() const noexcept;

    /// Where band `index` sits. Indices at `num_bands()` and beyond have no band.
    [[nodiscard]] units::Frequency band_center(std::size_t index) const noexcept;

    /// Lift or cut at band `index`, in decibels, clamped to what a band is allowed. An index
    /// with no band is remembered but does nothing.
    void set_band_gain(std::size_t index, float db) noexcept;

    [[nodiscard]] float band_gain(std::size_t index) const noexcept;

    /// The cut taken before the bands, in decibels, making room for what they add. Worked out
    /// from the band gains: what is lifted most is what has to be made room for.
    [[nodiscard]] float preamp() const noexcept;

    /// Whether the bands and the preamp run at all.
    void set_enabled(bool enabled) noexcept;

    [[nodiscard]] bool enabled() const noexcept;

    /// Applies the current settings to whole frames of `samples`, in place.
    void process(std::span<float> samples) noexcept override;

    /// Whether the last `process` changed anything, and so could have lifted a sample past what
    /// an output takes.
    [[nodiscard]] bool shaping() const noexcept;

    /// Whether the bands it just applied could have lifted a sample past full scale.
    [[nodiscard]] bool lifted() const noexcept override;

private:
    struct Bands;

    /// Gives every band the coefficients its gain now asks for.
    void retune() noexcept;

    StreamSpec spec_{};
    BandLayout layout_{};
    std::size_t num_bands_{0};
    std::unique_ptr<Bands> bands_;
};

} // namespace wiola::audio
