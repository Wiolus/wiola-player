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

#include <audio/stream_spec.hpp>
#include <utils/units.hpp>

#include <array>
#include <chrono>
#include <span>

namespace wiola::engine {

namespace {

using namespace std::chrono_literals;
using namespace units::literals;

/// How long to wait when the buffer is full, or when it is draining at the end.
constexpr auto poll_interval{2ms};

/// A quarter second of slack between the producer and the callback.
constexpr auto buffered{250_ms};

/// Frames handed from the decoder to the buffer at a time.
constexpr std::size_t block_size{1024};

} // namespace

Player::Player(codec::Decoder& source)
    : source_{&source}
    , buffer_{source.spec().samples_per(buffered)}
    , device_{source.spec(), buffer_}
{
}

Player::~Player()
{
    stop();
    wait();
}

bool Player::start()
{
    prime();

    if (!device_.start())
        return false;

    thread_ = std::jthread{[this] { run(); }};

    return true;
}

void Player::pause() noexcept
{
    device_.stop();
}

bool Player::resume() noexcept
{
    return device_.start();
}

bool Player::playing() const noexcept
{
    return device_.running();
}

void Player::stop() noexcept
{
    stopping_.store(true, std::memory_order_relaxed);
}

bool Player::finished() const noexcept
{
    return finished_.load(std::memory_order_acquire);
}

void Player::wait()
{
    if (thread_.joinable())
        thread_.join();
}

std::size_t Player::num_underruns() const noexcept
{
    return device_.num_underruns();
}

void Player::prime()
{
    std::array<float, block_size> chunk{};

    while (buffer_.size_approx() + chunk.size() <= buffer_.capacity()) {
        const std::size_t num_rendered{source_->render(chunk)};

        if (num_rendered == 0)
            break;

        buffer_.push(std::span{chunk}.first(num_rendered));
    }
}

void Player::run()
{
    const auto stopping = [this] {
        return stopping_.load(std::memory_order_relaxed);
    };
    std::array<float, block_size> chunk{};

    for (std::size_t num_rendered{source_->render(chunk)}; num_rendered > 0 && !stopping();
        num_rendered = source_->render(chunk)) {
        for (std::size_t num_written = 0; num_written < num_rendered && !stopping();) {
            num_written +=
                buffer_.push(std::span{chunk}.subspan(num_written, num_rendered - num_written));

            if (num_written < num_rendered)
                std::this_thread::sleep_for(poll_interval);
        }
    }

    // Reaching the end of the source leaves the buffer still to be heard; being stopped does not.
    while (!stopping() && buffer_.size_approx() > 0)
        std::this_thread::sleep_for(poll_interval);

    device_.stop();
    finished_.store(true, std::memory_order_release);
}

} // namespace wiola::engine
