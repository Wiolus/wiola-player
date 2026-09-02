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

#include "tag_reader.hpp"

#include <utility>

namespace wiola::engine {

TagReader::~TagReader()
{
    if (thread_.joinable()) {
        thread_.request_stop();
        waiting_.notify_all();
        thread_.join();
    }
}

void TagReader::start(std::vector<std::filesystem::path> paths)
{
    if (paths.empty())
        return;

    {
        const std::lock_guard asked{mutex_};

        for (std::filesystem::path& path : paths)
            asked_.push_back(std::move(path));
    }

    if (!thread_.joinable())
        thread_ = std::jthread{[this](const std::stop_token& stop) { work(stop); }};

    waiting_.notify_one();
}

std::vector<TagReader::Reading> TagReader::take()
{
    const std::lock_guard asked{mutex_};

    return std::exchange(read_, {});
}

bool TagReader::busy() const noexcept
{
    const std::lock_guard asked{mutex_};

    return !asked_.empty() || reading_;
}

void TagReader::work(const std::stop_token& stop)
{
    while (true) {
        std::filesystem::path path;

        {
            std::unique_lock asked{mutex_};

            waiting_.wait(asked, stop, [this] { return !asked_.empty(); });

            if (stop.stop_requested())
                return;

            path = std::move(asked_.front());
            asked_.pop_front();
            reading_ = true;
        }

        // Held by nothing while the file is read, which is the whole point of being here.
        codec::Tags tags{codec::read_tags(path)};

        {
            const std::lock_guard asked{mutex_};

            reading_ = false;
            read_.push_back(Reading{.path = std::move(path), .tags = std::move(tags)});
        }
    }
}

} // namespace wiola::engine
