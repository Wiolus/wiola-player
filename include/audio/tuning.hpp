/**
 * @file
 * @brief Values the output chain is built with.
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

#include <utils/units.hpp>

#include <cstddef>

namespace wiola::audio::tuning {

/// Where the lowest band sits: the exact value behind the 31 Hz it is called.
inline constexpr units::Frequency first_band_center{31.25};

/// How many bands a layout names, before a format has said which of them it can carry.
inline constexpr std::size_t num_layout_bands{10};

/// How far apart the bands are. Two is one band per octave.
inline constexpr double band_ratio{2.0};

/// How narrow each band is. Lower than the 1.41 an octave measures, so that bands lifted together
/// sum to within 0.75 dB rather than 1.7 at the price of reaching further.
inline constexpr double band_quality{1.0};

/// Highest center a band can be given, as a share of the sample rate. Above it, Nyquist has taken
/// the band's upper half.
inline constexpr double highest_band_share{0.4};

/// The most a band may lift or cut, in decibels.
inline constexpr double max_band_gain_db{12.0};

} // namespace wiola::audio::tuning
