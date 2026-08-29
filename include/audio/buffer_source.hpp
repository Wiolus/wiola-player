/**
 * @file
 * @brief Frames another thread has already decoded.
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

#include <audio/source.hpp>
#include <audio/stream_spec.hpp>
#include <lockfree/spsc_ring_buffer.hpp>

#include <cstddef>
#include <span>

namespace wiola::audio {

/// What a ring buffer holds. Short whenever whoever fills it has not kept up.
class BufferSource final : public Source {
public:
    BufferSource(StreamSpec spec, lockfree::SPSCRingBuffer<float>::Consumer buffer) noexcept;

    std::size_t render(std::span<float> interleaved) override;

    [[nodiscard]] StreamSpec spec() const noexcept override;

private:
    StreamSpec spec_;
    lockfree::SPSCRingBuffer<float>::Consumer buffer_;
};

} // namespace wiola::audio
