/**
 * @file
 * @brief A sine tone for tests, to play through something real.
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

#include <audio/dsp/source.hpp>
#include <audio/stream_spec.hpp>
#include <utils/units.hpp>

#include <cmath>
#include <cstddef>
#include <numbers>
#include <span>

namespace wiola::testing {

/// Generates a constant-frequency sine, the same signal on every channel.
class SineSource final : public audio::Source {
public:
    SineSource(audio::StreamSpec spec, units::Frequency frequency, float amplitude = 0.2F) noexcept;

    /// Writes whole frames only: channels alternate within a frame, never one array per channel
    /// as a planar layout would. A partial trailing frame is left untouched. Returns how many
    /// samples were written, which for a tone is every whole frame the span holds.
    std::size_t render(std::span<float> interleaved) noexcept override;
    [[nodiscard]] audio::StreamSpec spec() const noexcept override;

private:
    const audio::StreamSpec spec_{};
    double phase_{0.0};
    double phase_step_;
    float amplitude_;
};

inline constexpr double two_pi{2.0 * std::numbers::pi};

inline SineSource::SineSource(audio::StreamSpec spec, units::Frequency frequency,
    float amplitude) noexcept
    : spec_{spec}
    , phase_step_{two_pi * (frequency / spec.sample_rate)}
    , amplitude_{amplitude}
{
}

inline std::size_t SineSource::render(std::span<float> interleaved) noexcept
{
    const std::size_t num_channels{spec_.num_channels};
    std::size_t num_written{0};

    for (std::size_t i = 0; i + num_channels <= interleaved.size(); i += num_channels) {
        const auto value = static_cast<float>(amplitude_ * std::sin(phase_));

        for (std::size_t channel = 0; channel < num_channels; ++channel)
            interleaved[i + channel] = value;

        phase_ += phase_step_;

        if (phase_ >= two_pi)
            phase_ -= two_pi;

        num_written += num_channels;
    }

    return num_written;
}

inline audio::StreamSpec SineSource::spec() const noexcept
{
    return spec_;
}

} // namespace wiola::testing
