/**
 * @file
 * @brief A player wired the way a session wires one, with the sound card left out.
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

#include <audio/buffer_source.hpp>
#include <codec/decoder.hpp>
#include <engine/player.hpp>
#include <fakes/output.hpp>
#include <lockfree/spsc_ring_buffer.hpp>
#include <utils/units.hpp>

#include <chrono>
#include <memory>
#include <utility>

namespace wiola::testing {

/// What a player is given, wired the way a session wires it, with the sound card left out. Its
/// output pulls, so a track plays through as fast as it decodes: a test asking whether something
/// is still playing wants an output of its own instead.
struct Rig {
    explicit Rig(std::unique_ptr<codec::Decoder> source)
        : buffer{source->spec().samples_per(units::Time{std::chrono::milliseconds{250}})}
        , decoded{source->spec(), buffer.consumer()}
        , output{decoded}
        , player{std::move(source), buffer.producer(), output}
    {
    }

    lockfree::SPSCRingBuffer<float> buffer;
    audio::BufferSource decoded;
    FakeOutput output;
    engine::Player player;
};

} // namespace wiola::testing
