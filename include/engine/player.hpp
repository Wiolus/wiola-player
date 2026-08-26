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

#include <audio/chain.hpp>
#include <audio/device.hpp>
#include <codec/decoder.hpp>
#include <core/macros.hpp>
#include <lockfree/spsc_ring_buffer.hpp>
#include <utils/units.hpp>

#include <atomic>
#include <cstddef>
#include <memory>
#include <thread>

namespace wiola::engine {

/**
 * What the transport is doing.
 *
 * This is decided here rather than read back from the device. The device can stop on its own -
 * an output can be lost, or an audio server can give up on a stream - and that is a fault to be
 * noticed, not a change of what the listener asked for.
 *
 * `ended` and `stopped` are both final and differ in why: a source that ran out is the cue to
 * play the next thing, while a listener who pressed stop is not.
 */
enum class PlayerState {
    idle,
    playing,
    paused,
    ended,
    stopped,
};

/**
 * Plays one decoder, feeding it to a device from a thread of its own.
 *
 * Decoding may stall on a disk for as long as it likes, while the device wakes on a deadline and
 * must be answered at once. The two cannot share a thread, so the decoding happens here and the
 * caller's thread is left free to do something else - to notice a keypress, or to draw.
 *
 * The player owns its source and outlives nothing with it: the thread that reads the source is
 * stopped before the source is destroyed, which is a promise no caller can be asked to keep.
 */
class Player final {
public:
    /// Takes the source it plays, which must not be null, and what shapes its output. The chain
    /// is told the source's format and outlives the player.
    Player(std::unique_ptr<codec::Decoder> source, audio::Chain& chain);

    NO_COPY_SEMANTIC(Player);
    NO_MOVE_SEMANTIC(Player);

    /// Stops playback and waits for the thread, so a player is never destroyed while it runs.
    ~Player();

    /// Begins playing, from the beginning of the source or from wherever `seek` was last asked
    /// for while nothing was playing. A player that has already finished is wound back, so
    /// playing a track again is this same call. False when no device could be started, or when
    /// playback is already under way.
    [[nodiscard]] bool start();

    /// Silences playback, keeping the position and everything already decoded, so that resuming
    /// is immediate. False when there was nothing playing to silence.
    [[nodiscard]] bool pause() noexcept;

    /// Plays on from where `pause` left off. False unless the player is paused, or when the
    /// output could not be started again. Resuming is not a way to begin.
    [[nodiscard]] bool resume() noexcept;

    /// Ends playback at once, without playing out what is already buffered. Does nothing if
    /// playback has already finished, and may be called from any thread.
    void stop() noexcept;

    /// Moves playback to `position`, measured from the start of the source. Takes effect on the
    /// decoding thread rather than at once, and a position beyond the end is ignored. Whether
    /// sound is being produced is unchanged: seeking while paused stays paused, and seeking a
    /// player that has stopped or finished waits for the next `start` rather than being lost.
    void seek(units::Time position) noexcept;

    /// Blocks until playback has ended.
    void wait();

    /// What the transport is doing.
    [[nodiscard]] PlayerState state() const noexcept;

    /// Whether the transport is playing. Says nothing about whether the device agrees; when the
    /// two disagree the output has failed underneath us.
    [[nodiscard]] bool playing() const noexcept;

    /// Whether playback is over, however it ended. `state()` says which.
    [[nodiscard]] bool finished() const noexcept;

    /// How long the source runs, from its beginning to its end.
    [[nodiscard]] units::Time total_time() const noexcept;

    /// How far playback has reached, measured from the start of the source. Follows what is
    /// being heard rather than what has been decoded, so it moves with the device. A seek that
    /// has been asked for but not yet applied reads as the place it asked for.
    [[nodiscard]] units::Time time_played() const noexcept;

    /// Callbacks that found the buffer short and had to emit silence.
    [[nodiscard]] std::size_t num_underruns() const noexcept;

private:
    /// Fills the buffer before the device is started, so the first callbacks find frames waiting.
    void prime();

    /// The decoding thread: keeps the buffer fed until the source is spent.
    void run();

    /// Performs a requested seek, discarding what was decoded for the old position.
    void apply_seek();

    /// Whether a seek has been asked for and not yet carried out.
    [[nodiscard]] bool seek_outstanding() const noexcept;

    /// Settles on a final state. A listener who asked to stop outranks a source that ran out, so
    /// whichever happened first is what stays.
    void finish(PlayerState reason) noexcept;

    std::unique_ptr<codec::Decoder> source_;
    std::atomic<PlayerState> state_{PlayerState::idle};
    /// Seeks asked for, against seeks the decoding thread has carried out. While the two differ
    /// a request is outstanding, and where it asked to go is where playback is taken to be.
    std::atomic<std::size_t> seeks_requested_{0};
    std::atomic<std::size_t> seeks_applied_{0};
    std::atomic<std::size_t> seek_target_{0};
    std::atomic<std::size_t> position_base_{0};

    /// Samples handed to the buffer since the device last had its count reset. Playback is over
    /// when the device has played all of them, which is a fact rather than a buffer level.
    std::size_t num_pushed_{0};
    lockfree::SPSCRingBuffer<float> buffer_;

    /// Reads `buffer_`, so it is built before the device that asks it for frames.
    std::unique_ptr<audio::Source> output_;
    audio::Device device_;
    std::jthread thread_;
};

} // namespace wiola::engine
