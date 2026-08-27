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

#pragma once

#include <atomic>
#include <cstddef>

namespace wiola::engine {

/**
 * Where playback is, in frames from the start of the source.
 *
 * A place asked for is where playback is until it is reached, not the place left behind. Asking
 * and reading are for any thread; claiming, beginning and pushing are for the one that decodes.
 */
class Playhead {
public:
    /// A seek to carry out, read as one so that a request arriving after it stays outstanding.
    struct Claim {
        std::size_t requested{0};
        std::size_t target{0};
        bool outstanding{false};
    };

    /// Asks for playback to move to `frame_index`. Any thread.
    void request_seek(std::size_t frame_index) noexcept;

    /// Whether a seek has been asked for and not yet carried out.
    [[nodiscard]] bool seek_outstanding() const noexcept;

    /// What a seek would have to carry out now.
    [[nodiscard]] Claim claim() const noexcept;

    /// Playback starts again at `frame_index`, carrying out what `claim` held. What was counted
    /// before no longer applies.
    void begin_at(std::size_t frame_index, const Claim& claim) noexcept;

    /// Samples handed to the output since playback last began somewhere.
    void push(std::size_t num_samples) noexcept;

    [[nodiscard]] std::size_t num_pushed() const noexcept;

    /// Where playback is, given what the output says it has played.
    [[nodiscard]] std::size_t position(std::size_t frames_played) const noexcept;

private:
    std::atomic<std::size_t> seeks_requested_{0};
    std::atomic<std::size_t> seeks_applied_{0};
    std::atomic<std::size_t> seek_target_{0};
    std::atomic<std::size_t> base_{0};
    std::size_t num_pushed_{0};
};

} // namespace wiola::engine
