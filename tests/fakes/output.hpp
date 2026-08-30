/**
 * @file
 * @brief One output for tests to play through, drive and interrogate.
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

#include <audio/output.hpp>
#include <audio/source.hpp>
#include <audio/stream_spec.hpp>

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cstddef>
#include <mutex>
#include <thread>
#include <vector>

namespace wiola::testing {

/**
 * An output with no sound card behind it, for a test to play through and then ask about.
 *
 * It comes in two kinds, chosen by how it is built. Given a source it pulls on a thread of its
 * own, as fast as it is answered, so a track is played through in the time it takes to decode
 * it. Given nothing it plays nothing, and only a test saying `play` moves its count on.
 *
 * Which to use is not a detail: a test that asks whether something is still playing must build
 * one that plays nothing, or the track will have ended before the question is asked. That is a
 * fast machine failing a test that a slow one passes.
 *
 * It can also be made to behave like a device that has been lost, by refusing to start or by
 * stopping without being asked, and it can say which threads have started or stopped it.
 */
class FakeOutput final : public audio::Output {
public:
    /// Plays nothing. What has been heard is whatever a test says it has.
    FakeOutput() noexcept = default;

    /// Pulls `source` while it runs, so a track plays through as fast as it decodes.
    explicit FakeOutput(audio::Source& source) noexcept
        : source_{&source}
    {
    }

    ~FakeOutput() override
    {
        running_.store(false);

        if (thread_.joinable())
            thread_.join();
    }

    /// Says that `num_frames` more have been heard. For an output that pulls nothing.
    void play(audio::Frames num_frames) noexcept { frames_.fetch_add(num_frames.count()); }

    /// Every start from here on fails, the way a device that has gone away does.
    void refuse_starts() noexcept { refusing_.store(true); }

    /// Stops answering, without anyone having asked it to.
    void die() noexcept { running_.store(false); }

    /// From here on, remember which threads start or stop it.
    void watch() noexcept { watching_.store(true); }

    [[nodiscard]] std::vector<std::thread::id> touching_threads() const
    {
        const std::lock_guard watched{mutex_};

        return threads_;
    }

    [[nodiscard]] int num_starts() const noexcept { return num_starts_.load(); }

    [[nodiscard]] int num_stops() const noexcept { return num_stops_.load(); }

    [[nodiscard]] bool running() const noexcept override { return running_.load(); }

    [[nodiscard]] audio::Frames frames_played() const noexcept override
    {
        return audio::Frames{frames_.load()};
    }

private:
    bool start() noexcept override
    {
        note();
        num_starts_.fetch_add(1);

        if (refusing_.load())
            return false;

        if (running_.exchange(true))
            return true;

        if (source_ != nullptr) {
            if (thread_.joinable())
                thread_.join();

            thread_ = std::thread{[this] { pull(); }};
        }

        return true;
    }

    void stop() noexcept override
    {
        note();
        num_stops_.fetch_add(1);
        running_.store(false);

        if (thread_.joinable())
            thread_.join();
    }

    void pull()
    {
        const audio::StreamSpec spec{source_->spec()};
        std::array<float, 512> block{};

        while (running_.load()) {
            const std::size_t num_rendered{source_->render(block)};

            if (num_rendered == 0) {
                std::this_thread::sleep_for(std::chrono::milliseconds{1});
                continue;
            }

            frames_.fetch_add(spec.frames_per(num_rendered).count());
        }
    }

    void note() noexcept
    {
        if (!watching_.load())
            return;

        const std::lock_guard watched{mutex_};

        if (std::ranges::find(threads_, std::this_thread::get_id()) == threads_.end())
            threads_.push_back(std::this_thread::get_id());
    }

    audio::Source* source_{nullptr};

    std::atomic<bool> running_{false};
    std::atomic<bool> refusing_{false};
    std::atomic<std::size_t> frames_{0};
    std::atomic<int> num_starts_{0};
    std::atomic<int> num_stops_{0};

    std::atomic<bool> watching_{false};
    mutable std::mutex mutex_;
    std::vector<std::thread::id> threads_;

    std::thread thread_;
};

} // namespace wiola::testing
