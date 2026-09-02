/**
 * @file
 * @brief What is done to samples between the buffer and the output.
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

#include <audio/stage.hpp>
#include <audio/stream_spec.hpp>
#include <core/macros.hpp>

#include <memory>
#include <span>
#include <utility>
#include <vector>

namespace wiola::audio {

/**
 * What every sample passes through on its way to the output, in order.
 *
 * `process` is called from the thread that feeds the device: it does not allocate, lock or block.
 * Settings are written from another thread and take effect on the next call, never partway
 * through one.
 *
 * A chain belongs to the output rather than to a track, so loading one resets nothing.
 */
class Chain {
public:
    Chain() noexcept = default;

    NO_COPY_SEMANTIC(Chain);
    NO_MOVE_SEMANTIC(Chain);

    ~Chain() = default;

    /// Builds a step of type `T` from `args` and puts it at the end, where it will run last.
    /// Answers with the step itself, so whoever composed the chain can still reach the one it
    /// has something to ask of. Not the playing thread's.
    template<typename T, typename... Args>
    T& add(Args&&... args)
    {
        auto stage = std::make_unique<T>(std::forward<Args>(args)...);
        T& added{*stage};

        stages_.push_back(std::move(stage));

        return added;
    }

    /// Retunes every step for a stream of `spec`. Between tracks, never during one.
    void configure(StreamSpec spec);

    /// Runs every step over `samples`, in the order they were added, and puts back whatever a
    /// step lifted past what an output takes.
    void process(std::span<float> samples) noexcept;

private:
    std::vector<std::unique_ptr<Stage>> stages_;
};

} // namespace wiola::audio
