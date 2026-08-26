/**
 * @file
 * @brief Sine tone generator used to exercise the playback path.
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
#include <utils/units.hpp>

#include <cstddef>
#include <span>

namespace wiola::audio {

/// Generates a constant-frequency sine, the same signal on every channel.
class SineSource final : public Source {
public:
    SineSource(StreamSpec spec, units::Frequency frequency, float amplitude = 0.2F) noexcept;

    /// Writes whole frames only: channels alternate within a frame, never one array per channel
    /// as a planar layout would. A partial trailing frame is left untouched. Returns how many
    /// samples were written, which for a tone is every whole frame the span holds.
    std::size_t render(std::span<float> interleaved) noexcept override;
    [[nodiscard]] StreamSpec spec() const noexcept override;

private:
    const StreamSpec spec_{};
    double phase_{0.0};
    double phase_step_;
    float amplitude_;
};

} // namespace wiola::audio
