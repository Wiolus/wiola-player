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

#include "decode_loop.hpp"

#include <engine/tuning.hpp>

#include <audio/stream_spec.hpp>

#include <algorithm>
#include <array>
#include <chrono>
#include <span>
#include <thread>
#include <utility>

namespace wiola::engine {

DecodeLoop::DecodeLoop(std::unique_ptr<codec::Decoder> source,
    lockfree::SPSCRingBuffer<float>::Producer buffer, audio::Output::Control control,
    const audio::Output& output, Playback& playback, Playhead::Applier applier) noexcept
    : source_{std::move(source)}
    , buffer_{std::move(buffer)}
    , control_{std::move(control)}
    , output_{output}
    , playback_{playback}
    , applier_{std::move(applier)}
{
}

bool DecodeLoop::begin(bool rewind)
{
    // A seek asked for while nothing was decoding has no thread to take it up, so it waits here
    // instead of being thrown away: a listener who moves the slider before pressing play means
    // to begin there.
    const Playhead::Claim claim{applier_.claim()};
    const audio::Frames start_frame{
        claim.outstanding ? std::min(claim.target, source_->num_frames()) : audio::Frames{}};

    // A source that has never played is already at the beginning with nothing buffered.
    if (rewind || claim.outstanding) {
        buffer_.mark_discard();
        source_->seek(start_frame);
    }

    // Measured from what the output has counted so far, since a count it keeps itself is one
    // nothing else may wind back.
    applier_.begin_at(start_frame, output_.frames_played(), claim);

    prime();

    if (!control_.start())
        return false;

    output_started_ = true;

    return true;
}

void DecodeLoop::prime()
{
    std::array<float, tuning::decode_chunk_samples> block{};

    while (buffer_.space_approx() >= block.size()) {
        const std::size_t num_decoded{source_->render(block)};

        if (num_decoded == 0)
            break;

        applier_.push(buffer_.push(std::span{block}.first(num_decoded)));
    }
}

void DecodeLoop::apply_seek()
{
    const Playhead::Claim claim{applier_.claim()};

    // What is buffered belongs to the old position. Marking it stale is all the decoding side
    // can do: the space comes free when the output has stepped over it.
    stop_output();
    buffer_.mark_discard();

    source_->seek(claim.target);

    // The output is stopped, so what it has counted stands still while it is read.
    applier_.begin_at(claim.target, output_.frames_played(), claim);

    prime();

    // Whatever was asked for while the seek was being carried out.
    follow_playback();
}

void DecodeLoop::stop_output() noexcept
{
    if (!output_started_)
        return;

    control_.stop();
    output_started_ = false;
}

void DecodeLoop::follow_playback()
{
    const bool wanted{playback_.playing()};

    if (wanted == output_started_)
        return;

    if (!wanted) {
        stop_output();
        return;
    }

    if (control_.start()) {
        output_started_ = true;
        return;
    }

    // An output that will not start is a fault rather than a pause, and leaves nothing to play
    // through.
    playback_.finish(Playback::State::faulted);
}

void DecodeLoop::run()
{
    std::array<float, tuning::decode_chunk_samples> block{};

    // What the last decode left, and how much of it has gone into the buffer. A block that would
    // not fit is kept here rather than waited out, so nothing below is ever held up by more than
    // one decode.
    std::size_t num_decoded{0};
    std::size_t num_handed{0};
    bool exhausted{false};

    // Any final state ends this: a listener's stop, a device that went away, or the end of the
    // track below.
    while (!playback_.finished()) {
        // Every turn, because a listener's thread only moves the state: this is the one place a
        // pause reaches the device, and the only one before the waits below.
        follow_playback();

        // A device that was started and is no longer running was not stopped by anything here.
        // Waiting on one that has gone away is how playback looks like it is still going with
        // nothing coming out of it.
        if (output_started_ && !output_.running()) {
            playback_.finish(Playback::State::faulted);
            break;
        }

        if (applier_.seek_outstanding()) {
            apply_seek();

            // The block was decoded at the place that has just been left.
            num_decoded = 0;
            num_handed = 0;
            exhausted = false;
        }

        // Only once the last block has gone over in full, so that what would not fit is handed
        // over rather than decoded again.
        if (!exhausted && num_handed == num_decoded) {
            num_decoded = source_->render(block);
            num_handed = 0;
            exhausted = num_decoded == 0;
        }

        if (exhausted) {
            // Reaching the end of the source leaves what is buffered still to be heard; being
            // stopped does not, and being paused is neither - it waits to be resumed.
            if (played_out())
                break;

            std::this_thread::sleep_for(tuning::decode_poll_interval);
            continue;
        }

        const std::size_t num_left{num_decoded - num_handed};
        const std::size_t num_taken{buffer_.push(std::span{block}.subspan(num_handed, num_left))};

        // A full buffer is a wait on the device rather than on the source, and the one above is
        // the same wait: both are answered by the poll interval and by nothing else.
        if (num_taken == 0) {
            std::this_thread::sleep_for(tuning::decode_poll_interval);
            continue;
        }

        num_handed += num_taken;

        // The playhead counts what was handed over, not what was decoded: it is what the device
        // will play, and `played_out` above is measured against it.
        applier_.push(num_taken);
    }

    stop_output();
    playback_.finish(Playback::State::ended);
}

bool DecodeLoop::played_out() const noexcept
{
    // A device that was never started, or that a pause has silenced, still has what is buffered
    // left to play.
    if (!output_started_)
        return false;

    return output_.frames_played() >= source_->spec().frames_per(applier_.num_pushed());
}

} // namespace wiola::engine
