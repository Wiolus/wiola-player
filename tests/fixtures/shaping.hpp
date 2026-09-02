/**
 * @file
 * @brief A chain composed the way a session composes one.
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

#include <audio/chain.hpp>
#include <audio/equalizer.hpp>
#include <audio/stream_spec.hpp>
#include <audio/volume.hpp>

namespace wiola::testing {

/// The steps a session puts a track through, in the same order, with each of them to hand: the
/// bands first, then how loud, so that what a listener asks for last is done last.
struct Shaping {
    explicit Shaping(audio::StreamSpec spec, audio::BandLayout layout = {})
        : equalizer{chain.add<audio::Equalizer>(spec, layout)}
        , volume{chain.add<audio::Volume>()}
    {
    }

    audio::Chain chain;
    audio::Equalizer& equalizer;
    audio::Volume& volume;
};

} // namespace wiola::testing
