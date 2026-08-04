/**
 * @file
 * @brief Plays one decoder through a device.
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

#include <audio/device.hpp>
#include <codec/decoder.hpp>
#include <lockfree/spsc_ring_buffer.hpp>

#include <cstddef>
#include <thread>

namespace wiola::engine {

/**
 * Plays one decoder, feeding it to a device from a thread of its own.
 *
 * Decoding may stall on a disk for as long as it likes, while the device wakes on a deadline and
 * must be answered at once. The two cannot share a thread, so the decoding happens here and the
 * caller's thread is left free to do something else - to notice a keypress, or to draw.
 *
 * `source` must outlive the player.
 */
class Player final {
public:
    explicit Player(codec::Decoder& source);

    Player(const Player&) = delete;
    Player& operator=(const Player&) = delete;
    Player(Player&&) = delete;
    Player& operator=(Player&&) = delete;

    /// Waits for playback to finish, so a player is never destroyed while its thread still runs.
    ~Player();

    /// Begins playing. False when no device could be started, in which case nothing was played
    /// and the player must not be waited on.
    [[nodiscard]] bool start();

    /// Blocks until the source has been played to its end.
    void wait();

    /// Callbacks that found the buffer short and had to emit silence.
    [[nodiscard]] std::size_t num_underruns() const noexcept;

private:
    /// Fills the buffer before the device is started, so the first callbacks find frames waiting.
    void prime();

    /// The decoding thread: keeps the buffer fed until the source is spent.
    void run();

    codec::Decoder* source_;
    lockfree::SPSCRingBuffer<float> buffer_;
    audio::Device device_;
    std::jthread thread_;
};

} // namespace wiola::engine
