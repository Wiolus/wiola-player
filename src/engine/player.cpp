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

#include <engine/player.hpp>

#include "tuning.hpp"

#include <audio/stream_spec.hpp>
#include <utils/units.hpp>

#include <algorithm>
#include <array>
#include <chrono>
#include <span>

namespace wiola::engine {

Player::Player(std::unique_ptr<codec::Decoder> source, lockfree::SPSCRingBuffer<float>& buffer,
    audio::Output& output)
    : source_{std::move(source)}
    , buffer_{buffer}
    , output_{output}
{
}

Player::~Player()
{
    stop();
    wait();
}

bool Player::start()
{
    const Playback::State previous{playback_.state()};

    if (previous == Playback::State::playing || previous == Playback::State::paused)
        return false;

    // A player that has already run is wound back rather than thrown away, so playing a finished
    // track again is the same call as playing it the first time.
    if (thread_.joinable())
        thread_.join();

    // A seek asked for while nothing was decoding has no thread to take it up, so it waits here
    // instead of being thrown away: a listener who moves the slider before pressing play means
    // to begin there.
    const Playhead::Claim claim{head_.claim()};
    const audio::Frames start_frame{
        claim.outstanding ? std::min(claim.target, source_->num_frames()) : audio::Frames{}};

    // A player that has never run is already at the beginning with nothing buffered.
    if (previous != Playback::State::idle || claim.outstanding) {
        buffer_.mark_discard();
        source_->seek(start_frame);
    }

    // Measured from what the output has counted so far, since a count it keeps itself is one
    // nothing else may wind back.
    head_.begin_at(start_frame, output_.frames_played(), claim);

    prime();

    if (!output_.start())
        return false;

    output_started_ = true;

    if (!playback_.begin())
        return false;

    thread_ = std::jthread{[this] { run(); }};

    return true;
}

void Player::seek(units::Time position) noexcept
{
    const double frames{source_->spec().sample_rate * std::max(position, units::Time{})};

    head_.request_seek(audio::Frames{static_cast<std::size_t>(frames)});
}

bool Player::pause() noexcept
{
    return playback_.pause();
}

bool Player::resume() noexcept
{
    return playback_.resume();
}

units::Time Player::total_time() const noexcept
{
    return source_->spec().time_per(source_->num_frames());
}

units::Time Player::time_played() const noexcept
{
    return source_->spec().time_per(head_.position(output_.frames_played()));
}

bool Player::playing() const noexcept
{
    return playback_.playing();
}

void Player::stop() noexcept
{
    playback_.finish(Playback::State::stopped);
}

Playback::State Player::state() const noexcept
{
    return playback_.state();
}

bool Player::finished() const noexcept
{
    return playback_.finished();
}

void Player::wait()
{
    if (thread_.joinable())
        thread_.join();
}

void Player::prime()
{
    std::array<float, tuning::decode_chunk_samples> block{};

    while (buffer_.size_approx() + block.size() <= buffer_.capacity()) {
        const std::size_t num_decoded{source_->render(block)};

        if (num_decoded == 0)
            break;

        head_.push(buffer_.push(std::span{block}.first(num_decoded)));
    }
}

void Player::apply_seek()
{
    const Playhead::Claim claim{head_.claim()};

    // What is buffered belongs to the old position. Marking it stale is all the decoding side
    // can do: the space comes free when the output has stepped over it.
    stop_output();
    buffer_.mark_discard();

    source_->seek(claim.target);

    // The output is stopped, so what it has counted stands still while it is read.
    head_.begin_at(claim.target, output_.frames_played(), claim);

    prime();

    // Whatever was asked for while the seek was being carried out.
    follow_playback();
}

void Player::stop_output() noexcept
{
    if (!output_started_)
        return;

    output_.stop();
    output_started_ = false;
}

void Player::follow_playback()
{
    const bool wanted{playback_.playing()};

    if (wanted == output_started_)
        return;

    if (!wanted) {
        stop_output();
        return;
    }

    if (output_.start()) {
        output_started_ = true;
        return;
    }

    // An output that will not start is a fault rather than a pause, and leaves nothing to play
    // through.
    playback_.finish(Playback::State::stopped);
}

void Player::run()
{
    std::array<float, tuning::decode_chunk_samples> block{};

    // What the last decode left, and how much of it has gone into the buffer. A block that would
    // not fit is kept here rather than waited out, so nothing below is ever held up by more than
    // one decode.
    std::size_t num_decoded{0};
    std::size_t num_handed{0};
    bool exhausted{false};

    while (playback_.state() != Playback::State::stopped) {
        // Every turn, because a listener's thread only moves the state: this is the one place a
        // pause reaches the device, and the only one before the waits below.
        follow_playback();

        if (head_.seek_outstanding()) {
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
        head_.push(num_taken);
    }

    stop_output();
    playback_.finish(Playback::State::ended);
}

bool Player::played_out() const noexcept
{
    // A device that was never started, or that a pause has silenced, still has what is buffered
    // left to play.
    if (!output_started_)
        return false;

    // An output that has died says so by no longer running, which is the difference between
    // waiting and hanging.
    if (!output_.running())
        return true;

    const audio::StreamSpec spec{source_->spec()};

    return output_.frames_played() >= spec.frames_per(head_.num_pushed());
}

} // namespace wiola::engine
