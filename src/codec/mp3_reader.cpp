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

#include "file_stream.hpp"

#define DR_MP3_IMPLEMENTATION
#include <dr_mp3.h>

#include <array>
#include <utility>

namespace wiola::codec {

struct Mp3Reader::Handle {
    std::unique_ptr<FileStream> stream;
    drmp3 mp3{};
    bool open{false};

    ~Handle()
    {
        if (open)
            drmp3_uninit(&mp3);
    }
};

namespace {

using Callbacks = StreamCallbacks<drmp3_seek_origin, DRMP3_SEEK_SET, DRMP3_SEEK_CUR, drmp3_int64>;

} // namespace

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
    handle->stream = FileStream::open(path);

    if (!handle->stream)
        return nullptr;

    if (drmp3_init(&handle->mp3, Callbacks::read, Callbacks::seek, Callbacks::tell, nullptr,
            handle->stream.get(), nullptr) == DRMP3_FALSE)
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

bool Mp3Reader::seek_frame(std::size_t frame_index)
{
    return drmp3_seek_to_pcm_frame(&handle_->mp3, frame_index) == DRMP3_TRUE;
}

const Format& Mp3Reader::format()
{
    // No markers: an MP3 frame opens with eleven set bits, which arbitrary data lands on often
    // enough that it identifies nothing. MP3 is recognized by being tried, not by being seen.
    static constexpr std::array<std::string_view, 3> extensions{".mp3", ".mp2", ".mpga"};
    static const Format format{"MP3", {}, extensions,
        [](const std::filesystem::path& path) -> std::unique_ptr<Decoder> {
            return Mp3Reader::open(path);
        }};

    return format;
}

} // namespace wiola::codec
