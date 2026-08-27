/**
 * @file
 * @brief Where playback is, and where it has been asked to go.
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

#include <engine/playhead.hpp>

namespace wiola::engine {

void Playhead::request_seek(audio::Frames frame_index) noexcept
{
    seek_target_.store(frame_index.count(), std::memory_order_relaxed);
    seeks_requested_.fetch_add(1, std::memory_order_release);
}

bool Playhead::seek_outstanding() const noexcept
{
    return seeks_requested_.load(std::memory_order_acquire) !=
        seeks_applied_.load(std::memory_order_relaxed);
}

Playhead::Claim Playhead::claim() const noexcept
{
    const std::size_t requested{seeks_requested_.load(std::memory_order_acquire)};

    return Claim{
        .requested = requested,
        .target = audio::Frames{seek_target_.load(std::memory_order_relaxed)},
        .outstanding = requested != seeks_applied_.load(std::memory_order_relaxed),
    };
}

void Playhead::begin_at(audio::Frames frame_index, const Claim& claim) noexcept
{
    base_.store(frame_index.count(), std::memory_order_relaxed);
    num_pushed_ = 0;

    // Carried out only now that the place asked for is the place reported.
    seeks_applied_.store(claim.requested, std::memory_order_release);
}

void Playhead::push(std::size_t num_samples) noexcept
{
    num_pushed_ += num_samples;
}

std::size_t Playhead::num_pushed() const noexcept
{
    return num_pushed_;
}

audio::Frames Playhead::position(audio::Frames frames_played) const noexcept
{
    if (seek_outstanding())
        return audio::Frames{seek_target_.load(std::memory_order_relaxed)};

    return audio::Frames{base_.load(std::memory_order_relaxed)} + frames_played;
}

} // namespace wiola::engine
