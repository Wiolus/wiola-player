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

#include <audio/chain.hpp>
#include <audio/device.hpp>
#include <audio/stream_spec.hpp>
#include <audio/tone.hpp>
#include <lockfree/spsc_ring_buffer.hpp>
#include <utils/units.hpp>

#include <gtest/gtest.h>

#include <array>
#include <chrono>
#include <thread>

namespace {

using wiola::audio::Chain;
using wiola::audio::Device;
using wiola::audio::DeviceState;
using wiola::audio::SineSource;
using namespace wiola::units;
using wiola::audio::StreamSpec;
using wiola::lockfree::SPSCRingBuffer;

constexpr StreamSpec stereo{.sample_rate = 48_kHz, .num_channels = 2};

TEST(Device, ReportsItsSpecBeforeStarting)
{
    SPSCRingBuffer<float> buffer{stereo.samples_per(100_ms)};
    Chain chain{stereo};
    Device device{stereo, buffer, chain};

    EXPECT_EQ(device.spec(), stereo);
    EXPECT_EQ(device.state(), DeviceState::closed);
    EXPECT_FALSE(device.running());
    EXPECT_EQ(device.num_underruns(), 0u);
}

/// Stopping gives back the callback, not the output, so starting again does not reopen anything.
TEST(Device, KeepsTheOutputBetweenStops)
{
    SPSCRingBuffer<float> buffer{stereo.samples_per(100_ms)};
    Chain chain{stereo};
    Device device{stereo, buffer, chain};

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

TEST(Device, DrainsTheRingBufferWhileRunning)
{
    SPSCRingBuffer<float> buffer{stereo.samples_per(250_ms)};
    Chain chain{stereo};
    Device device{stereo, buffer, chain};
    SineSource source{stereo, 440_Hz};

    std::array<float, 1024> chunk{};
    while (buffer.size_approx() + chunk.size() <= buffer.capacity()) {
        source.render(chunk);
        buffer.push(chunk);
    }

    const std::size_t num_queued = buffer.size_approx();

    if (!device.start()) {
        GTEST_SKIP() << "no playback device on this machine";
    }

    EXPECT_TRUE(device.running());
    std::this_thread::sleep_for(std::chrono::milliseconds{120});
    device.stop();

    EXPECT_FALSE(device.running());
    EXPECT_LT(buffer.size_approx(), num_queued);
}

} // namespace
