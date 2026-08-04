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

#include <array>
#include <utility>

namespace wiola::codec {

struct FlacReader::Handle {
    drflac* flac{nullptr};

    ~Handle()
    {
        if (flac != nullptr)
            drflac_close(flac);
    }
};

FlacReader::FlacReader(audio::StreamSpec spec, std::size_t num_frames,
    std::unique_ptr<Handle> handle) noexcept
    : Decoder{spec, num_frames}
    , handle_{std::move(handle)}
{
}

FlacReader::~FlacReader() = default;

std::unique_ptr<FlacReader> FlacReader::open(const std::filesystem::path& path)
{
    drflac* flac{drflac_open_file(path.c_str(), nullptr)};

    if (flac == nullptr)
        return nullptr;

    const audio::StreamSpec spec{.sample_rate = units::Frequency{flac->sampleRate},
        .num_channels = flac->channels};
    const auto num_frames = static_cast<std::size_t>(flac->totalPCMFrameCount);

    return std::unique_ptr<FlacReader>{
        new FlacReader{spec, num_frames, std::make_unique<Handle>(flac)}
    };
}

std::size_t FlacReader::decode(std::span<float> output, std::size_t num_frames)
{
    return static_cast<std::size_t>(drflac_read_pcm_frames_f32(handle_->flac, num_frames,
        output.data()));
}

bool FlacReader::seek_frame(std::size_t frame_index)
{
    return drflac_seek_to_pcm_frame(handle_->flac, frame_index) == DRFLAC_TRUE;
}

const Format& FlacReader::format()
{
    static constexpr std::array markers{
        Marker{0, "fLaC"}
    };

    static constexpr std::array<std::string_view, 1> extensions{".flac"};
    static const Format format{"FLAC", markers, extensions,
        [](const std::filesystem::path& path) -> std::unique_ptr<Decoder> {
            return FlacReader::open(path);
        }};

    return format;
}

} // namespace wiola::codec
