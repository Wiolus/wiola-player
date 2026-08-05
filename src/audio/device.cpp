/**
 * @file
 * @brief Playback device fed by a single-producer ring buffer.
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

#include <miniaudio.h>

#include <algorithm>
#include <span>

namespace wiola::audio {

struct Device::Backend {
    ma_device handle{};
    bool initialized{false};
};

Device::Device(StreamSpec spec, lockfree::SPSCRingBuffer<float>& buffer)
    : spec_{spec}
    , buffer_{&buffer}
    , backend_{std::make_unique<Backend>()}
{
}

Device::~Device()
{
    stop();

    if (backend_->initialized)
        ma_device_uninit(&backend_->handle);
}

void Device::render(std::span<float> output) noexcept
{
    const std::size_t num_popped{buffer_->pop(output)};

    if (num_popped < output.size()) {
        std::ranges::fill(output.subspan(num_popped), 0.0F);
        num_underruns_.fetch_add(1, std::memory_order_relaxed);
    }
}

bool Device::start() noexcept
{
    if (backend_->initialized)
        return ma_device_start(&backend_->handle) == MA_SUCCESS;

    ma_device_config config{ma_device_config_init(ma_device_type_playback)};
    config.playback.format = ma_format_f32;
    config.playback.channels = static_cast<ma_uint32>(spec_.num_channels);
    config.sampleRate = static_cast<ma_uint32>(spec_.sample_rate.get<units::Hz>());
    config.pUserData = this;
    config.dataCallback = [](ma_device* handle, void* output, const void*, ma_uint32 num_frames) {
        auto* device = static_cast<Device*>(handle->pUserData);
        const std::size_t num_samples{device->spec_.samples_per(num_frames)};
        device->render(std::span<float>{static_cast<float*>(output), num_samples});
    };

    if (ma_device_init(nullptr, &config, &backend_->handle) != MA_SUCCESS)
        return false;

    if (ma_device_start(&backend_->handle) != MA_SUCCESS) {
        ma_device_uninit(&backend_->handle);
        return false;
    }

    backend_->initialized = true;
    return true;
}

void Device::stop() noexcept
{
    if (running())
        ma_device_stop(&backend_->handle);
}

bool Device::running() const noexcept
{
    return backend_->initialized && ma_device_is_started(&backend_->handle) == MA_TRUE;
}

StreamSpec Device::spec() const noexcept
{
    return spec_;
}

std::size_t Device::num_underruns() const noexcept
{
    return num_underruns_.load(std::memory_order_relaxed);
}

} // namespace wiola::audio
