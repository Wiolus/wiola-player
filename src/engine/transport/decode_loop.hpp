/**
 * @file
 * @brief Decodes a source into the buffer a device plays from, on a thread of its own.
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

#include "playhead.hpp"
#include <audio/output.hpp>
#include <codec/decoder.hpp>
#include <core/macros.hpp>
#include <engine/playback.hpp>
#include <lockfree/spsc_ring_buffer.hpp>

#include <memory>

namespace wiola::engine {

/**
 * Keeps the buffer a device plays from fed, and keeps the device doing what playback says.
 *
 * The whole audio path is held here and nowhere else: the source, the end of the buffer that
 * fills it, the end of the output that drives it, and the end of the playhead that carries seeks
 * out. Whoever holds one of these has no way to reach any of them, which is what keeps a
 * listener's thread out of a device that this one is driving.
 *
 * `begin` is the caller's, before there is a thread. Everything after it belongs to the thread
 * that runs `run`.
 */
class DecodeLoop {
public:
    /// Takes the source it plays, the ends of the buffer, output and playhead it works through,
    /// the output it reads what has been heard from, and the playback it follows. All of them
    /// outlive it.
    DecodeLoop(std::unique_ptr<codec::Decoder> source,
        lockfree::SPSCRingBuffer<float>::Producer buffer, audio::Output::Control control,
        const audio::Output& output, Playback& playback, Playhead::Applier applier) noexcept;

    NO_COPY_SEMANTIC(DecodeLoop);
    NO_MOVE_SEMANTIC(DecodeLoop);
    ~DecodeLoop() = default;

    /// Winds the source to wherever a seek asked for, fills the buffer and starts the device.
    /// `rewind` when playback has run before, so that what is buffered and where the source sits
    /// are both left belonging to the place about to be played. False when no device started.
    [[nodiscard]] bool begin(bool rewind);

    /// Feeds the device until the source is spent and what it left has been heard, or until
    /// playback is stopped. One thread runs this, and it is the only one that may.
    void run();

private:
    /// Fills the buffer as far as it will go, so the first callbacks find frames waiting.
    void prime();

    /// Carries out a requested seek, marking what was decoded for the old place as stale.
    void apply_seek();

    /// Starts or stops the output so that it does what playback says.
    void follow_playback();

    /// Silences the output if it was started.
    void stop_output() noexcept;

    /// Whether the device has played everything handed to it, or has died trying. False while
    /// anything is still to be heard, which a pause counts as.
    [[nodiscard]] bool played_out() const noexcept;

    std::unique_ptr<codec::Decoder> source_;
    lockfree::SPSCRingBuffer<float>::Producer buffer_;
    audio::Output::Control control_;
    const audio::Output& output_;
    Playback& playback_;
    Playhead::Applier applier_;

    /// What the output was last asked for. This thread's, and seeded by `begin` before there is
    /// a thread at all.
    bool output_started_{false};
};

} // namespace wiola::engine
