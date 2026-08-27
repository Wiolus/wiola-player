/**
 * @file
 * @brief The volume as the window drives it.
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

#include "volume_control.hpp"

#include "tuning.hpp"

#include <algorithm>
#include <cmath>

namespace wiola::gui {

namespace {

/// Under which name the position is kept.
constexpr auto position_key{"volume"};

/// The gain a position asks for. The curve is what keeps the audible range off the bottom of the
/// slider's travel.
float gain_of(int percent) noexcept
{
    const double position{static_cast<double>(percent) / tuning::full_volume};

    return static_cast<float>(std::pow(position, tuning::volume_curve));
}

} // namespace

VolumeControl::VolumeControl(audio::Volume& volume, QSettings& settings) noexcept
    : volume_{volume}
    , settings_{settings}
    , position_{tuning::full_volume}
{
}

int VolumeControl::restore()
{
    bool numeric{false};
    const int stored{settings_.value(position_key, tuning::full_volume).toInt(&numeric)};

    set_position(numeric ? std::clamp(stored, 0, tuning::full_volume) : tuning::full_volume);

    return position_;
}

void VolumeControl::set_position(int percent)
{
    position_ = std::clamp(percent, 0, tuning::full_volume);

    volume_.set_gain(gain_of(position_));
    settings_.setValue(position_key, position_);
}

int VolumeControl::position() const noexcept
{
    return position_;
}

} // namespace wiola::gui
