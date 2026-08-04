/**
 * @file
 * @brief Reader for FLAC files.
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

#include "flac_reader.hpp"

#define DR_FLAC_IMPLEMENTATION
#include <dr_flac.h>

namespace wiola::codec {

struct FlacReader::Handle {
    drflac* flac{nullptr};

    ~Handle()
    {
        if (flac != nullptr)
            drflac_close(flac);
    }
};

FlacReader::~FlacReader() = default;

std::unique_ptr<FlacReader> FlacReader::open(const std::filesystem::path& path)
{
    drflac* flac{drflac_open_file(path.c_str(), nullptr)};

    if (flac == nullptr)
        return nullptr;

    std::unique_ptr<FlacReader> reader{new FlacReader};
    reader->handle_ = std::make_unique<Handle>(flac);
    reader->spec_ = audio::StreamSpec{.sample_rate = units::Frequency{flac->sampleRate},
        .num_channels = flac->channels};
    reader->num_frames_ = static_cast<std::size_t>(flac->totalPCMFrameCount);

    return reader;
}

std::size_t FlacReader::render(std::span<float> interleaved)
{
    const std::size_t num_channels{spec_.num_channels};
    const std::size_t num_frames_wanted{
        std::min(interleaved.size() / num_channels, num_frames_left())};

    if (num_frames_wanted == 0)
        return 0;

    const std::size_t num_frames_read{static_cast<std::size_t>(
        drflac_read_pcm_frames_f32(handle_->flac, num_frames_wanted, interleaved.data())
    )};

    num_frames_read_ += num_frames_read;

    return num_frames_read * num_channels;
}

audio::StreamSpec FlacReader::spec() const noexcept
{
    return spec_;
}

std::size_t FlacReader::num_frames() const noexcept
{
    return num_frames_;
}

std::size_t FlacReader::num_frames_left() const noexcept
{
    return num_frames_ - num_frames_read_;
}

} // namespace wiola::codec
