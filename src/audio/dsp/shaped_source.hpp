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

#pragma once

#include <audio/dsp/chain.hpp>
#include <audio/dsp/source.hpp>
#include <audio/stream_spec.hpp>

#include <cstddef>
#include <span>

namespace wiola::audio {

/// What another source gives, shaped on its way out. Both outlive it.
class ShapedSource final : public Source {
public:
    ShapedSource(Source& source, Chain& chain) noexcept;

    std::size_t render(std::span<float> interleaved) override;

    [[nodiscard]] StreamSpec spec() const noexcept override;

private:
    Source& source_;
    Chain& chain_;
};

} // namespace wiola::audio
