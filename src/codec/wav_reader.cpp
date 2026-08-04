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

#include <array>
#include <utility>

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

WavReader::WavReader(audio::StreamSpec spec, std::size_t num_frames,
    std::unique_ptr<Handle> handle) noexcept
    : Decoder{spec, num_frames}
    , handle_{std::move(handle)}
{
}

WavReader::~WavReader() = default;

std::unique_ptr<WavReader> WavReader::open(const std::filesystem::path& path)
{
    auto handle = std::make_unique<Handle>();

    if (drwav_init_file(&handle->wav, path.c_str(), nullptr) == DRWAV_FALSE)
        return nullptr;

    handle->open = true;

    const audio::StreamSpec spec{.sample_rate = units::Frequency{handle->wav.sampleRate},
        .num_channels = handle->wav.channels};
    const auto num_frames = static_cast<std::size_t>(handle->wav.totalPCMFrameCount);

    return std::unique_ptr<WavReader>{
        new WavReader{spec, num_frames, std::move(handle)}
    };
}

std::size_t WavReader::decode(std::span<float> output, std::size_t num_frames)
{
    return static_cast<std::size_t>(drwav_read_pcm_frames_f32(&handle_->wav, num_frames,
        output.data()));
}

bool WavReader::seek_frame(std::size_t frame_index)
{
    return drwav_seek_to_pcm_frame(&handle_->wav, frame_index) == DRWAV_TRUE;
}

const Format& WavReader::format()
{
    static constexpr std::array markers{
        Marker{0, "RIFF"},
        Marker{8, "WAVE"}
    };

    static constexpr std::array<std::string_view, 2> extensions{".wav", ".wave"};
    static const Format format{"WAV", markers, extensions,
        [](const std::filesystem::path& path) -> std::unique_ptr<Decoder> {
            return WavReader::open(path);
        }};

    return format;
}

} // namespace wiola::codec
