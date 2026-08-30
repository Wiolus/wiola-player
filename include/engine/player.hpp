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

#include <audio/output.hpp>
#include <audio/stream_spec.hpp>
#include <codec/decoder.hpp>
#include <core/macros.hpp>
#include <engine/decode_loop.hpp>
#include <engine/playback.hpp>
#include <engine/playhead.hpp>
#include <lockfree/spsc_ring_buffer.hpp>
#include <utils/units.hpp>

#include <memory>
#include <thread>

namespace wiola::engine {

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
    /// Takes the source it plays, which must not be null, the buffer it decodes into, and the
    /// output that buffer is played through. Both outlive the player.
    Player(std::unique_ptr<codec::Decoder> source, lockfree::SPSCRingBuffer<float>::Producer buffer,
        audio::Output& output);

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
    /// is immediate. The device is silenced by the decoding thread rather than here, so a caller
    /// is never held up by one. False when there was nothing playing to silence.
    [[nodiscard]] bool pause() noexcept;

    /// Plays on from where `pause` left off, again by way of the decoding thread. False unless
    /// the player is paused. Resuming is not a way to begin, and an output that will not start
    /// again ends playback rather than being reported here: `state()` reads as stopped.
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

    /// What playback is doing.
    [[nodiscard]] Playback::State state() const noexcept;

    /// Whether a track is playing. Says nothing about whether the device agrees; when the
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

private:
    Playback playback_;
    Playhead head_;

    /// Read from, never driven: starting and stopping it is the decoding loop's.
    audio::Output& output_;

    /// The source's own, taken once. What a track is does not change while it is loaded, and
    /// asking the source would mean reaching into what the loop below owns.
    audio::StreamSpec spec_;
    audio::Frames num_frames_;

    /// The audio path, whole. Nothing above this line can reach into it.
    DecodeLoop loop_;

    std::jthread thread_;
};

} // namespace wiola::engine
