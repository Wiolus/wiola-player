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

#include "mp3_reader.hpp"

#define DR_MP3_IMPLEMENTATION
#include <dr_mp3.h>

#include <utility>

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

Mp3Reader::Mp3Reader(audio::StreamSpec spec, std::size_t num_frames,
    std::unique_ptr<Handle> handle) noexcept
    : Decoder{spec, num_frames}
    , handle_{std::move(handle)}
{
}

Mp3Reader::~Mp3Reader() = default;

std::unique_ptr<Mp3Reader> Mp3Reader::open(const std::filesystem::path& path)
{
    auto handle = std::make_unique<Handle>();

    if (drmp3_init_file(&handle->mp3, path.c_str(), nullptr) == DRMP3_FALSE)
        return nullptr;

    handle->open = true;

    const audio::StreamSpec spec{.sample_rate = units::Frequency{handle->mp3.sampleRate},
        .num_channels = handle->mp3.channels};
    const auto num_frames = static_cast<std::size_t>(drmp3_get_pcm_frame_count(&handle->mp3));

    return std::unique_ptr<Mp3Reader>{
        new Mp3Reader{spec, num_frames, std::move(handle)}
    };
}

std::size_t Mp3Reader::decode(std::span<float> output, std::size_t num_frames)
{
    return static_cast<std::size_t>(drmp3_read_pcm_frames_f32(&handle_->mp3, num_frames,
        output.data()));
}

} // namespace wiola::codec
