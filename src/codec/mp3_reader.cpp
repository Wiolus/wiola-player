/**
 * @file
 * @brief Reader for MP3 files.
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

#include <codec/mp3_reader.hpp>

#define DR_MP3_IMPLEMENTATION
#include <dr_mp3.h>

namespace wiola::codec {

struct Mp3Reader::Handle {
    drmp3 mp3{};
    bool open{false};

    ~Handle()
    {
        if (open)
            drmp3_uninit(&mp3);
    }
};

Mp3Reader::~Mp3Reader() = default;

std::unique_ptr<Mp3Reader> Mp3Reader::open(const std::filesystem::path& path)
{
    auto handle = std::make_unique<Handle>();

    if (drmp3_init_file(&handle->mp3, path.c_str(), nullptr) == DRMP3_FALSE)
        return nullptr;

    handle->open = true;

    std::unique_ptr<Mp3Reader> reader{new Mp3Reader};
    reader->spec_ = audio::StreamSpec{.sample_rate = units::Frequency{handle->mp3.sampleRate},
        .num_channels = handle->mp3.channels};
    reader->num_frames_ = static_cast<std::size_t>(drmp3_get_pcm_frame_count(&handle->mp3));
    reader->handle_ = std::move(handle);

    return reader;
}

std::size_t Mp3Reader::render(std::span<float> interleaved)
{
    const std::size_t num_channels{spec_.num_channels};
    const std::size_t num_frames_wanted{
        std::min(interleaved.size() / num_channels, num_frames_left())};

    if (num_frames_wanted == 0)
        return 0;

    const std::size_t num_frames_read{static_cast<std::size_t>(
        drmp3_read_pcm_frames_f32(&handle_->mp3, num_frames_wanted, interleaved.data())
    )};

    num_frames_read_ += num_frames_read;

    return num_frames_read * num_channels;
}

audio::StreamSpec Mp3Reader::spec() const noexcept
{
    return spec_;
}

std::size_t Mp3Reader::num_frames() const noexcept
{
    return num_frames_;
}

std::size_t Mp3Reader::num_frames_left() const noexcept
{
    return num_frames_ - num_frames_read_;
}

} // namespace wiola::codec
