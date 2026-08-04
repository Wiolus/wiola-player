/**
 * @file
 * @brief Reader for PCM audio stored in a RIFF/WAVE file.
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

#include "wav_reader.hpp"

#define DR_WAV_IMPLEMENTATION
#include <dr_wav.h>

namespace wiola::codec {

struct WavReader::Handle {
    drwav wav{};
    bool open{false};

    ~Handle()
    {
        if (open)
            drwav_uninit(&wav);
    }
};

WavReader::~WavReader() = default;

std::unique_ptr<WavReader> WavReader::open(const std::filesystem::path& path)
{
    auto handle = std::make_unique<Handle>();

    if (drwav_init_file(&handle->wav, path.c_str(), nullptr) == DRWAV_FALSE)
        return nullptr;

    handle->open = true;

    std::unique_ptr<WavReader> reader{new WavReader};
    reader->spec_ = audio::StreamSpec{.sample_rate = units::Frequency{handle->wav.sampleRate},
        .num_channels = handle->wav.channels};
    reader->num_frames_ = static_cast<std::size_t>(handle->wav.totalPCMFrameCount);
    reader->handle_ = std::move(handle);

    return reader;
}

std::size_t WavReader::render(std::span<float> interleaved)
{
    const std::size_t num_channels{spec_.num_channels};
    const std::size_t num_frames_wanted{
        std::min(interleaved.size() / num_channels, num_frames_left())};

    if (num_frames_wanted == 0)
        return 0;

    const std::size_t num_frames_read{static_cast<std::size_t>(
        drwav_read_pcm_frames_f32(&handle_->wav, num_frames_wanted, interleaved.data())
    )};

    num_frames_read_ += num_frames_read;

    return num_frames_read * num_channels;
}

audio::StreamSpec WavReader::spec() const noexcept
{
    return spec_;
}

std::size_t WavReader::num_frames() const noexcept
{
    return num_frames_;
}

std::size_t WavReader::num_frames_left() const noexcept
{
    return num_frames_ - num_frames_read_;
}

} // namespace wiola::codec
