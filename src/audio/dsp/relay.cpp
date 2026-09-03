/**
 * @file
 * @brief A source that plays whatever it has been pointed at.
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

#include <audio/dsp/relay.hpp>

namespace wiola::audio {

Relay::Relay(StreamSpec spec) noexcept
    : spec_{spec}
{
}

void Relay::point_at(Source& source) noexcept
{
    source_.store(&source, std::memory_order_release);
}

void Relay::point_at_nothing() noexcept
{
    source_.store(nullptr, std::memory_order_release);
}

bool Relay::pointed() const noexcept
{
    return source_.load(std::memory_order_acquire) != nullptr;
}

std::size_t Relay::render(std::span<float> interleaved)
{
    Source* source{source_.load(std::memory_order_acquire)};

    return source != nullptr ? source->render(interleaved) : 0;
}

StreamSpec Relay::spec() const noexcept
{
    return spec_;
}

} // namespace wiola::audio
