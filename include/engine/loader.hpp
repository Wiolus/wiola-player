/**
 * @file
 * @brief Turns a path into a decoder, off the thread that asked for it.
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

#include <codec/open.hpp>
#include <core/macros.hpp>

#include <condition_variable>
#include <cstddef>
#include <filesystem>
#include <mutex>
#include <optional>
#include <thread>

namespace wiola::engine {

/**
 * Opens files on a thread of its own.
 *
 * Opening one is not the quick thing it looks like: a stream that does not say how long it is
 * has to be read through to find out, which for a large track is seconds. A listener's thread
 * cannot wait for that and still draw, so it asks here and comes back for the answer.
 *
 * Asking and collecting are for the one thread that does them - the reading happens elsewhere,
 * and nothing of the file crosses back until it is whole.
 */
class Loader {
public:
    Loader() = default;

    NO_COPY_SEMANTIC(Loader);
    NO_MOVE_SEMANTIC(Loader);

    /// Waits for a file being opened, since what reads it would outlive what it reads into.
    ~Loader();

    /// Begins opening `path`. One already being opened is left to finish and its answer dropped:
    /// a listener who picks another file has said which one they meant.
    void start(const std::filesystem::path& path);

    /// Whether a file is being opened, or is waiting to be.
    [[nodiscard]] bool busy() const noexcept;

    /// The finished load, once. Nothing while one is still being opened, and nothing twice.
    [[nodiscard]] std::optional<codec::Opened> take();

private:
    void work(const std::stop_token& stop);

    mutable std::mutex mutex_;
    std::condition_variable_any waiting_;

    std::optional<std::filesystem::path> asked_;
    std::optional<codec::Opened> done_;

    /// How many loads have been asked for. An answer is kept only if nothing was asked for after
    /// the question it answers.
    std::size_t num_asked_{0};
    bool opening_{false};

    std::jthread thread_;
};

} // namespace wiola::engine
