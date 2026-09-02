/**
 * @file
 * @brief Reads what queued files say about themselves, off the thread that queued them.
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

#include <codec/tags.hpp>
#include <core/macros.hpp>

#include <condition_variable>
#include <deque>
#include <filesystem>
#include <mutex>
#include <thread>
#include <vector>

namespace wiola::engine {

/**
 * Reads what files say about themselves, on a thread of its own.
 *
 * A queue of a hundred tracks is a hundred files to open, and a listener who has just dropped
 * them in is watching. Each is quick, and a hundred of them are not, so they are read here and
 * collected afterwards.
 *
 * Asking and collecting are for the one thread that does them; the reading is elsewhere. What
 * has been read is answered once, in whatever order the files came back.
 */
class TagReader {
public:
    /// What a file turned out to say.
    struct Reading {
        std::filesystem::path path;
        codec::Tags tags;
    };

    TagReader() = default;

    NO_COPY_SEMANTIC(TagReader);
    NO_MOVE_SEMANTIC(TagReader);

    /// Waits for the file being read, since what reads it would outlive what it reads into.
    ~TagReader();

    /// Begins reading `paths`, after whatever was asked for before them.
    void start(std::vector<std::filesystem::path> paths);

    /// Everything read since the last time this was asked, and nothing twice.
    [[nodiscard]] std::vector<Reading> take();

    /// Whether anything is still to be read.
    [[nodiscard]] bool busy() const noexcept;

private:
    void work(const std::stop_token& stop);

    mutable std::mutex mutex_;
    std::condition_variable_any waiting_;

    std::deque<std::filesystem::path> asked_;
    std::vector<Reading> read_;
    bool reading_{false};

    std::jthread thread_;
};

} // namespace wiola::engine
