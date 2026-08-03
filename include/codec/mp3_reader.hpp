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

#pragma once

#include <audio/stream_spec.hpp>
#include <codec/decoder.hpp>

#include <cstddef>
#include <filesystem>
#include <memory>
#include <span>

namespace wiola::codec {

/// Decodes MP3 to float frames.
class Mp3Reader final : public Decoder {
public:
    /// Opens `path`. Null when the file is missing or is not MP3.
    static std::unique_ptr<Mp3Reader> open(const std::filesystem::path& path);

    ~Mp3Reader() override;

    std::size_t render(std::span<float> interleaved) override;

    [[nodiscard]] audio::StreamSpec spec() const noexcept override;
    [[nodiscard]] std::size_t num_frames() const noexcept override;
    [[nodiscard]] std::size_t num_frames_left() const noexcept override;

private:
    struct Handle;

    Mp3Reader() = default;

    std::unique_ptr<Handle> handle_;
    audio::StreamSpec spec_{};
    std::size_t num_frames_{0};
    std::size_t num_frames_read_{0};
};

} // namespace wiola::codec
