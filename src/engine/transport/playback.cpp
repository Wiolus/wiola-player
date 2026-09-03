/**
 * @file
 * @brief What playback is doing, and the rules by which that changes.
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

#include <engine/transport/playback.hpp>

namespace wiola::engine {

namespace {

/// The states nothing but `begin` moves out of.
bool settled(Playback::State state) noexcept
{
    return state == Playback::State::ended || state == Playback::State::stopped ||
        state == Playback::State::faulted;
}

} // namespace

Playback::State Playback::state() const noexcept
{
    return state_.load(std::memory_order_acquire);
}

bool Playback::playing() const noexcept
{
    return state() == State::playing;
}

bool Playback::finished() const noexcept
{
    return settled(state());
}

bool Playback::begin() noexcept
{
    State current{state_.load(std::memory_order_relaxed)};

    while (current != State::playing && current != State::paused)
        if (state_.compare_exchange_weak(current, State::playing, std::memory_order_acq_rel,
                std::memory_order_relaxed))
            return true;

    return false;
}

bool Playback::pause() noexcept
{
    // Claimed rather than stored: a stop or the end of the track may have taken playback out
    // of playing in the meantime, and a final state is not undone.
    State playing{State::playing};

    return state_.compare_exchange_strong(playing, State::paused, std::memory_order_acq_rel,
        std::memory_order_relaxed);
}

bool Playback::resume() noexcept
{
    State paused{State::paused};

    return state_.compare_exchange_strong(paused, State::playing, std::memory_order_acq_rel,
        std::memory_order_relaxed);
}

void Playback::finish(State reason) noexcept
{
    State current{state_.load(std::memory_order_relaxed)};

    while (!settled(current) &&
        !state_.compare_exchange_weak(current, reason, std::memory_order_acq_rel,
            std::memory_order_relaxed)) { }
}

} // namespace wiola::engine
