/**
 * @file
 * @brief Unit tests for the miniaudio playback device.
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

#include <audio/device.hpp>
#include <audio/source.hpp>
#include <audio/stream_spec.hpp>
#include <audio/tone.hpp>
#include <utils/units.hpp>

#include <gtest/gtest.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstddef>
#include <span>
#include <thread>

namespace {

using wiola::audio::Device;
using wiola::audio::DeviceState;
using wiola::audio::SineSource;
using namespace wiola::units;
using wiola::audio::Source;
using wiola::audio::StreamSpec;

constexpr StreamSpec stereo{.sample_rate = 48_kHz, .num_channels = 2};

/// Answers with silence and remembers what it was asked for, filling `fill_fraction` of every span.
class CountingSource final : public Source {
public:
    CountingSource(StreamSpec spec, double fill_fraction) noexcept
        : spec_{spec}
        , fill_fraction_{fill_fraction}
    {
    }

    std::size_t render(std::span<float> interleaved) override
    {
        const wiola::audio::Frames num_frames{static_cast<std::size_t>(
            static_cast<double>(spec_.frames_per(interleaved.size()).count()) * fill_fraction_
        )};
        const std::size_t num_rendered{spec_.samples_per(num_frames)};

        std::ranges::fill(interleaved, 0.0F);
        num_asked_.fetch_add(interleaved.size(), std::memory_order_relaxed);

        return num_rendered;
    }

    [[nodiscard]] StreamSpec spec() const noexcept override { return spec_; }

    [[nodiscard]] std::size_t num_asked() const noexcept
    {
        return num_asked_.load(std::memory_order_relaxed);
    }

private:
    StreamSpec spec_;
    double fill_fraction_;
    std::atomic<std::size_t> num_asked_{0};
};

TEST(Device, ReportsItsSpecBeforeStarting)
{
    SineSource tone{stereo, 440_Hz};
    Device device{tone};

    EXPECT_EQ(device.spec(), stereo);
    EXPECT_EQ(device.state(), DeviceState::closed);
    EXPECT_FALSE(device.running());
    EXPECT_EQ(device.num_underruns(), 0u);
}

/// Stopping gives back the callback, not the output, so starting again does not reopen anything.
TEST(Device, KeepsTheOutputBetweenStops)
{
    SineSource tone{stereo, 440_Hz};
    Device device{tone};

    if (!device.start())
        GTEST_SKIP() << "no playback device on this machine";

    EXPECT_EQ(device.state(), DeviceState::running);

    device.stop();
    EXPECT_EQ(device.state(), DeviceState::stopped);

    ASSERT_TRUE(device.start());
    EXPECT_EQ(device.state(), DeviceState::running);

    device.stop();
    device.stop();
    EXPECT_EQ(device.state(), DeviceState::stopped);
}

TEST(Device, AsksItsSourceForFramesWhileRunning)
{
    CountingSource source{stereo, 1.0};
    Device device{source};

    if (!device.start())
        GTEST_SKIP() << "no playback device on this machine";

    EXPECT_TRUE(device.running());
    std::this_thread::sleep_for(std::chrono::milliseconds{120});
    device.stop();

    EXPECT_FALSE(device.running());
    EXPECT_GT(source.num_asked(), 0u);
    EXPECT_GT(device.frames_played(), wiola::audio::Frames{});
    EXPECT_EQ(device.num_underruns(), 0u);
}

TEST(Device, CountsACallbackTheSourceCouldNotFill)
{
    CountingSource source{stereo, 0.5};
    Device device{source};

    if (!device.start())
        GTEST_SKIP() << "no playback device on this machine";

    std::this_thread::sleep_for(std::chrono::milliseconds{120});
    device.stop();

    EXPECT_GT(device.num_underruns(), 0u);
    EXPECT_LT(device.frames_played(), stereo.frames_per(source.num_asked()));
}

} // namespace
