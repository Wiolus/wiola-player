/**
 * @file
 * @brief Another source with the chain applied.
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

#include <audio/dsp/shaped_source.hpp>

namespace wiola::audio {

ShapedSource::ShapedSource(Source& source, Chain& chain) noexcept
    : source_{source}
    , chain_{chain}
{
}

std::size_t ShapedSource::render(std::span<float> interleaved)
{
    const std::size_t num_rendered{source_.render(interleaved)};

    chain_.process(interleaved.first(num_rendered));

    return num_rendered;
}

pcm::StreamSpec ShapedSource::spec() const noexcept
{
    return source_.spec();
}

} // namespace wiola::audio
