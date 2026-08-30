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

#include "player.hpp"

#include <audio/stream_spec.hpp>
#include <utils/units.hpp>

#include <algorithm>
#include <array>
#include <chrono>
#include <span>

namespace wiola::engine {

Player::Player(std::unique_ptr<codec::Decoder> source,
    lockfree::SPSCRingBuffer<float>::Producer buffer, audio::Output& output)
    : output_{output}
    , spec_{source->spec()}
    , num_frames_{source->num_frames()}
    , loop_{std::move(source), std::move(buffer), output.control(), output, playback_,
          head_.applier()}
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
    // track again is the same call as playing it the first time. The thread is joined before the
    // loop is asked for anything, since it is the loop that thread was running.
    if (thread_.joinable())
        thread_.join();

    if (!loop_.begin(previous != Playback::State::idle))
        return false;

    if (!playback_.begin())
        return false;

    thread_ = std::jthread{[this] { loop_.run(); }};

    return true;
}

void Player::seek(units::Time position) noexcept
{
    const double frames{spec_.sample_rate * std::max(position, units::Time{})};

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
    return spec_.time_per(num_frames_);
}

units::Time Player::time_played() const noexcept
{
    return spec_.time_per(head_.position(output_.frames_played()));
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

} // namespace wiola::engine
