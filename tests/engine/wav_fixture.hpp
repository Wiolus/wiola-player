/**
 * @file
 * @brief A WAV file for tests that need a track to play.
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

#include <codec/decoder.hpp>
#include <codec/open.hpp>

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <memory>
#include <numbers>
#include <string>

namespace wiola::testing {

/// Seconds of audio a test source holds. It has to exceed what a player buffers ahead, or priming
/// swallows the whole source before playback even starts and nothing can be observed.
inline constexpr double source_seconds{1.5};
inline constexpr std::uint32_t source_rate{44100};
inline constexpr std::uint16_t source_channels{2};

inline void append(std::string& out, std::uint32_t value, std::size_t num_bytes)
{
    for (std::size_t i = 0; i < num_bytes; ++i)
        out += static_cast<char>((value >> (8 * i)) & 0xFFU);
}

/// Writes a few seconds of tone as a WAV file, and gives back where it put it.
inline std::filesystem::path write_wav(const char* name)
{
    const auto num_frames{static_cast<std::size_t>(source_seconds * source_rate)};

    std::string samples;
    for (std::size_t i = 0; i < num_frames; ++i) {
        const auto value = static_cast<std::uint32_t>(static_cast<std::int16_t>(12000 *
            std::sin(2 * std::numbers::pi * 440 * static_cast<double>(i) / source_rate)));

        for (std::uint16_t channel = 0; channel < source_channels; ++channel)
            append(samples, value, 2);
    }

    std::string fmt;
    append(fmt, 1, 2);
    append(fmt, source_channels, 2);
    append(fmt, source_rate, 4);
    append(fmt, source_rate * source_channels * 2, 4);
    append(fmt, source_channels * 2, 2);
    append(fmt, 16, 2);

    std::string body{"WAVEfmt "};
    append(body, static_cast<std::uint32_t>(fmt.size()), 4);
    body += fmt + "data";
    append(body, static_cast<std::uint32_t>(samples.size()), 4);
    body += samples;

    std::string file{"RIFF"};
    append(file, static_cast<std::uint32_t>(body.size()), 4);
    file += body;

    const std::filesystem::path path{std::filesystem::temp_directory_path() / name};
    std::ofstream out{path, std::ios::binary};
    out.write(file.data(), static_cast<std::streamsize>(file.size()));
    out.close();

    return path;
}

/// The same file, opened.
inline std::unique_ptr<codec::Decoder> open_fixture(const char* name = "wiola_fixture.wav")
{
    return std::move(codec::open_file(write_wav(name)).decoder);
}

} // namespace wiola::testing
