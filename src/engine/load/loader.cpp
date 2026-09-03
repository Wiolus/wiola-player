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

#include "loader.hpp"

#include <utility>

namespace wiola::engine {

Loader::~Loader()
{
    if (thread_.joinable()) {
        thread_.request_stop();
        waiting_.notify_all();
        thread_.join();
    }
}

void Loader::start(const std::filesystem::path& path)
{
    {
        const std::lock_guard asked{mutex_};

        asked_ = path;
        done_.reset();
        ++num_asked_;
    }

    if (!thread_.joinable())
        thread_ = std::jthread{[this](const std::stop_token& stop) { work(stop); }};

    waiting_.notify_one();
}

bool Loader::busy() const noexcept
{
    const std::lock_guard asked{mutex_};

    return asked_.has_value() || opening_;
}

std::optional<codec::Opened> Loader::take()
{
    const std::lock_guard asked{mutex_};

    return std::exchange(done_, std::nullopt);
}

void Loader::work(const std::stop_token& stop)
{
    while (true) {
        std::filesystem::path path;
        std::size_t asked_when{0};

        {
            std::unique_lock asked{mutex_};

            waiting_.wait(asked, stop, [this] { return asked_.has_value(); });

            if (stop.stop_requested())
                return;

            path = *std::exchange(asked_, std::nullopt);
            asked_when = num_asked_;
            opening_ = true;
        }

        // Held by nothing while the file is read, which is the whole point of being here.
        codec::Opened opened{codec::open_file(path)};

        {
            const std::lock_guard asked{mutex_};

            opening_ = false;

            // Another file was asked for while this one was being read, so this answer is to a
            // question nobody is waiting for.
            if (asked_when == num_asked_)
                done_ = std::move(opened);
        }
    }
}

} // namespace wiola::engine
