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

#pragma once

#include <audio/stream_spec.hpp>
#include <codec/decoder.hpp>

#include <cstddef>
#include <filesystem>
#include <memory>
#include <span>

namespace wiola::codec {

/**
 * Decodes RIFF/WAVE to float frames.
 *
 * WAVE is a container, not one encoding: samples may be 8- to 32-bit integers, IEEE floats,
 * A-law, mu-law or ADPCM. Every one of those is accepted, and metadata chunks placed among the
 * audio are stepped over rather than mistaken for samples.
 */
class WavReader final : public Decoder {
public:
    /// Opens `path`. Null when the file is missing or is not WAVE.
    static std::unique_ptr<WavReader> open(const std::filesystem::path& path);

    ~WavReader() override;

    std::size_t render(std::span<float> interleaved) override;

    [[nodiscard]] audio::StreamSpec spec() const noexcept override;
    [[nodiscard]] std::size_t num_frames() const noexcept override;
    [[nodiscard]] std::size_t num_frames_left() const noexcept override;

private:
    struct Handle;

    WavReader() = default;

    std::unique_ptr<Handle> handle_;
    audio::StreamSpec spec_{};
    std::size_t num_frames_{0};
    std::size_t num_frames_read_{0};
};

} // namespace wiola::codec
