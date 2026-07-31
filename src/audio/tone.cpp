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

#include <audio/tone.hpp>

#include <cmath>
#include <numbers>

namespace wiola::audio {

namespace {

constexpr double two_pi{2.0 * std::numbers::pi};

} // namespace

SineSource::SineSource(StreamSpec spec, units::Frequency frequency, float amplitude) noexcept
    : spec_{spec}
    , phase_step_{two_pi * (frequency / spec.sample_rate)}
    , amplitude_{amplitude}
{
}

void SineSource::render(std::span<float> interleaved) noexcept
{
    const std::size_t num_channels{spec_.num_channels};

    for (std::size_t i = 0; i + num_channels <= interleaved.size(); i += num_channels) {
        const auto value = static_cast<float>(amplitude_ * std::sin(phase_));

        for (std::size_t channel = 0; channel < num_channels; ++channel)
            interleaved[i + channel] = value;

        phase_ += phase_step_;

        if (phase_ >= two_pi)
            phase_ -= two_pi;
    }
}

StreamSpec SineSource::spec() const noexcept
{
    return spec_;
}

} // namespace wiola::audio
