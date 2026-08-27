/**
 * @file
 * @brief The equalizer as the window drives it.
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
#include <utils/units.hpp>

#include <QSettings>

#include <cstddef>

namespace wiola::gui {

/// Settings applied to `equalizer` and kept for the runs after this one. Nothing else may set
/// them, or what they are set to is not what comes back.
class EqualizerControl {
public:
    EqualizerControl(audio::Equalizer& equalizer, QSettings& settings) noexcept;

    /// Applies what a previous run left, for every band the layout names.
    void restore();

    void set_enabled(bool enabled);
    void set_preamp(float db);
    void set_band_gain(std::size_t index, float db);

    [[nodiscard]] bool enabled() const noexcept;
    [[nodiscard]] float preamp() const noexcept;
    [[nodiscard]] float band_gain(std::size_t index) const noexcept;
    [[nodiscard]] std::size_t num_bands() const noexcept;
    [[nodiscard]] units::Frequency band_center(std::size_t index) const noexcept;

private:
    audio::Equalizer& equalizer_;
    QSettings& settings_;
};

} // namespace wiola::gui
