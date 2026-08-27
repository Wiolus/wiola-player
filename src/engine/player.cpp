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
    const PlayerState previous{state()};

    if (previous == PlayerState::playing || previous == PlayerState::paused)
        return false;

    // A player that has already run is wound back rather than thrown away, so playing a finished
    // track again is the same call as playing it the first time.
    if (thread_.joinable())
        thread_.join();

    // A seek asked for while nothing was decoding has no thread to take it up, so it waits here
    // instead of being thrown away: a listener who moves the slider before pressing play means
    // to begin there.
    const Playhead::Claim claim{head_.claim()};

    if (previous != PlayerState::idle || claim.outstanding) {
        const audio::Frames start_frame{
            claim.outstanding ? std::min(claim.target, source_->num_frames()) : audio::Frames{}};

        buffer_.clear();
        source_->seek(start_frame);
        output_.reset_frames_played();
        head_.begin_at(start_frame, claim);
    }

    prime();

    if (!output_.start())
        return false;

    state_.store(PlayerState::playing, std::memory_order_release);
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
    if (state() != PlayerState::playing)
        return false;

    output_.stop();
    state_.store(PlayerState::paused, std::memory_order_release);

    return true;
}

bool Player::resume() noexcept
{
    if (state() != PlayerState::paused)
        return false;

    if (!output_.start())
        return false;

    state_.store(PlayerState::playing, std::memory_order_release);

    return true;
}

units::Time Player::total_time() const noexcept
{
    return units::Time{static_cast<double>(source_->num_frames().count()) /
        source_->spec().sample_rate.get<units::Hz>()};
}

units::Time Player::time_played() const noexcept
{
    const audio::Frames frames{head_.position(output_.frames_played())};

    return units::Time{
        static_cast<double>(frames.count()) / source_->spec().sample_rate.get<units::Hz>()};
}

bool Player::playing() const noexcept
{
    return state() == PlayerState::playing;
}

void Player::stop() noexcept
{
    finish(PlayerState::stopped);
}

void Player::finish(PlayerState reason) noexcept
{
    PlayerState current{state_.load(std::memory_order_relaxed)};

    // Whoever reaches a final state first keeps it, so a stop during the last block is not undone
    // by the source running out a moment later.
    while (current != PlayerState::ended && current != PlayerState::stopped &&
        !state_.compare_exchange_weak(current, reason, std::memory_order_acq_rel,
            std::memory_order_relaxed)) { }
}

PlayerState Player::state() const noexcept
{
    return state_.load(std::memory_order_acquire);
}

bool Player::finished() const noexcept
{
    const PlayerState current{state()};

    return current == PlayerState::ended || current == PlayerState::stopped;
}

void Player::wait()
{
    if (thread_.joinable())
        thread_.join();
}

void Player::prime()
{
    std::array<float, tuning::decode_chunk_samples> chunk{};

    while (buffer_.size_approx() + chunk.size() <= buffer_.capacity()) {
        const std::size_t num_rendered{source_->render(chunk)};

        if (num_rendered == 0)
            break;

        head_.push(buffer_.push(std::span{chunk}.first(num_rendered)));
    }
}

void Player::apply_seek()
{
    const Playhead::Claim claim{head_.claim()};

    // What is buffered belongs to the old position. The consumer has to be stopped before it can
    // be thrown away, since discarding it moves an index the consumer owns.
    const bool was_playing{state() == PlayerState::playing};

    output_.stop();
    buffer_.clear();

    source_->seek(claim.target);

    output_.reset_frames_played();
    head_.begin_at(claim.target, claim);

    prime();

    if (was_playing && !output_.start())
        finish(PlayerState::stopped);
}

void Player::run()
{
    const auto stopping = [this] {
        return state() == PlayerState::stopped;
    };
    std::array<float, tuning::decode_chunk_samples> chunk{};

    const auto seeking = [this] {
        return head_.seek_outstanding();
    };

    while (!stopping()) {
        if (seeking())
            apply_seek();

        const std::size_t num_rendered{source_->render(chunk)};

        if (num_rendered == 0)
            break;

        for (std::size_t num_written = 0; num_written < num_rendered && !stopping();) {
            const std::size_t num_taken{
                buffer_.push(std::span{chunk}.subspan(num_written, num_rendered - num_written))};

            num_written += num_taken;
            head_.push(num_taken);

            if (num_written < num_rendered) {
                // A seek makes the rest of this block stale, so it is abandoned rather than
                // waited out - the buffer is full precisely when a listener is most likely to
                // be moving around in the track.
                if (seeking())
                    break;

                std::this_thread::sleep_for(tuning::decode_poll_interval);
            }
        }
    }

    // Reaching the end of the source leaves what is buffered still to be heard; being stopped does
    // not. The device's own count says when it has been, and an output that has died says so by
    // no longer running, which is the difference between waiting and hanging.
    const audio::StreamSpec spec{source_->spec()};

    while (!stopping() && output_.running() &&
        output_.frames_played() < spec.frames_per(head_.num_pushed()))
        std::this_thread::sleep_for(tuning::decode_poll_interval);

    output_.stop();
    finish(PlayerState::ended);
}

} // namespace wiola::engine
