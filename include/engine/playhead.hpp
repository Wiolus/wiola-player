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

#include <audio/stream_spec.hpp>

#include <core/macros.hpp>

#include <atomic>
#include <cstddef>
#include <utility>

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
        audio::Frames target{};
        bool outstanding{false};
    };

    /**
     * The end that carries seeks out and counts what has been handed to the output: the
     * decoding thread's.
     *
     * Kept apart from the rest because what it touches is not all atomic - two threads counting
     * pushes race on a plain number - while asking and reading are safe for anyone, and the
     * thread that draws a seek bar needs both.
     *
     * Moving it hands that work to another thread. What is moved from, and any applier taken
     * after the first, does nothing: it claims no seek and counts no push.
     */
    class Applier {
    public:
        NO_COPY_SEMANTIC(Applier);

        Applier(Applier&& other) noexcept
            : head_{std::exchange(other.head_, nullptr)}
        {
        }

        Applier& operator=(Applier&& other) noexcept
        {
            head_ = std::exchange(other.head_, nullptr);

            return *this;
        }

        ~Applier() = default;

        /// What a seek would have to carry out now.
        [[nodiscard]] Claim claim() const noexcept
        {
            return head_ != nullptr ? head_->claim() : Claim{};
        }

        /// Playback starts again at `frame_index`, carrying out what `claim` held.
        /// `frames_played` is what the output had counted by then, which is what the position is
        /// measured from afterwards - the output's own count is never wound back, since the
        /// thread that plays it is the only one that may write it. What was counted before no
        /// longer applies.
        void begin_at(audio::Frames frame_index, audio::Frames frames_played,
            const Claim& claim) noexcept
        {
            if (head_ != nullptr)
                head_->begin_at(frame_index, frames_played, claim);
        }

        /// Counts `num_samples` more as handed to the output.
        void push(std::size_t num_samples) noexcept
        {
            if (head_ != nullptr)
                head_->push(num_samples);
        }

        /// Samples handed to the output since playback last began somewhere.
        [[nodiscard]] std::size_t num_pushed() const noexcept
        {
            return head_ != nullptr ? head_->num_pushed() : 0;
        }

    private:
        friend class Playhead;

        Applier() noexcept = default;

        explicit Applier(Playhead& head) noexcept
            : head_{&head}
        {
        }

        Playhead* head_{nullptr};
    };

    /// The end that carries seeks out. There is one: whoever takes it first does that work, and
    /// every one taken after is inert.
    [[nodiscard]] Applier applier() noexcept
    {
        if (applier_taken_)
            return Applier{};

        applier_taken_ = true;

        return Applier{*this};
    }

    /// Asks for playback to move to `frame_index`. Any thread: this only says where to go.
    void request_seek(audio::Frames frame_index) noexcept;

    /// Whether a seek has been asked for and not yet carried out. Any thread.
    [[nodiscard]] bool seek_outstanding() const noexcept;

    /// Where playback is, given what the output says it has played. Any thread, and read as one
    /// so that a reader never pairs a place with a count from before playback began there.
    [[nodiscard]] audio::Frames position(audio::Frames frames_played) const noexcept;

private:
    friend class Applier;

    /// Carrying seeks out is the applier's, above. Called only from there.
    [[nodiscard]] Claim claim() const noexcept;
    void begin_at(audio::Frames frame_index, audio::Frames frames_played,
        const Claim& claim) noexcept;
    void push(std::size_t num_samples) noexcept;
    [[nodiscard]] std::size_t num_pushed() const noexcept;

    bool applier_taken_{false};

    std::atomic<std::size_t> seeks_requested_{0};
    std::atomic<std::size_t> seeks_applied_{0};
    std::atomic<std::size_t> seek_target_{0};
    std::atomic<std::size_t> base_{0};
    std::atomic<std::size_t> frames_at_begin_{0};
    std::size_t num_pushed_{0};
};

} // namespace wiola::engine
