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

#include <audio/buffer_source.hpp>

namespace wiola::audio {

BufferSource::BufferSource(StreamSpec spec, lockfree::SPSCRingBuffer<float>& buffer) noexcept
    : spec_{spec}
    , buffer_{buffer}
{
}

std::size_t BufferSource::render(std::span<float> interleaved)
{
    return buffer_.pop(interleaved);
}

StreamSpec BufferSource::spec() const noexcept
{
    return spec_;
}

} // namespace wiola::audio
