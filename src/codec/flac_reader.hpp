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

#pragma once

#include "format.hpp"

#include <audio/stream_spec.hpp>
#include <codec/decoder.hpp>

#include <cstddef>
#include <filesystem>
#include <memory>
#include <span>

namespace wiola::codec {

/// Decodes FLAC to float frames.
class FlacReader final : public Decoder {
public:
    /// How this format is recognized and opened.
    [[nodiscard]] static const Format& format();

    /// Opens `path`. Null when the file is missing or is not FLAC.
    [[nodiscard]] static std::unique_ptr<FlacReader> open(const std::filesystem::path& path);

    ~FlacReader() override;

private:
    struct Handle;

    FlacReader(audio::StreamSpec spec, std::size_t num_frames,
        std::unique_ptr<Handle> handle) noexcept;

    std::size_t decode(std::span<float> output, std::size_t num_frames) override;

    bool seek_frame(std::size_t frame_index) override;

    std::unique_ptr<Handle> handle_;
};

} // namespace wiola::codec
