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

#include <atomic>
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

    /// Stops playback and waits for the thread, so a player is never destroyed while it runs.
    ~Player();

    /// Begins playing. False when no device could be started, in which case nothing was played
    /// and the player must not be waited on.
    [[nodiscard]] bool start();

    /// Silences playback, keeping the position and everything already decoded, so that resuming
    /// is immediate. Doing this twice is the same as doing it once.
    void pause() noexcept;

    /// Plays on from where `pause` left off. False when the output could not be started again.
    [[nodiscard]] bool resume() noexcept;

    /// Whether sound is being produced. False while paused, and once playback has ended.
    [[nodiscard]] bool playing() const noexcept;

    /// Ends playback at once, without playing out what is already buffered. Does nothing if
    /// playback has already finished, and may be called from any thread.
    void stop() noexcept;

    /// Whether playback has ended, either by reaching the end of the source or by being stopped.
    [[nodiscard]] bool finished() const noexcept;

    /// Blocks until playback has ended.
    void wait();

    /// Callbacks that found the buffer short and had to emit silence.
    [[nodiscard]] std::size_t num_underruns() const noexcept;

private:
    /// Fills the buffer before the device is started, so the first callbacks find frames waiting.
    void prime();

    /// The decoding thread: keeps the buffer fed until the source is spent.
    void run();

    codec::Decoder* source_;
    std::atomic<bool> stopping_{false};
    std::atomic<bool> finished_{false};
    lockfree::SPSCRingBuffer<float> buffer_;
    audio::Device device_;
    std::jthread thread_;
};

} // namespace wiola::engine
