/**
 * @file
 * @brief One second-order section.
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

#include <audio/dsp/biquad.hpp>

#include <cmath>
#include <numbers>

namespace wiola::audio {

BiquadCoefficients BiquadCoefficients::peaking(pcm::StreamSpec spec, units::Frequency center,
    double quality, double gain_db) noexcept
{
    const double rate{spec.sample_rate.get<units::Hz>()};

    if (rate <= 0.0 || quality <= 0.0)
        return BiquadCoefficients{};

    // The peaking section of the audio EQ cookbook. Half the decibels go into `reach` because a
    // peak arrives at the gain while its skirts arrive at the square root of it.
    const double reach{std::pow(10.0, gain_db / 40.0)};
    const double angle{2.0 * std::numbers::pi * center.get<units::Hz>() / rate};
    const double width{std::sin(angle) / (2.0 * quality)};
    const double cosine{std::cos(angle)};

    // Everything is divided through by this, so that the section carries one number fewer.
    const double leading{1.0 + (width / reach)};

    return BiquadCoefficients{
        .b0 = static_cast<float>((1.0 + (width * reach)) / leading),
        .b1 = static_cast<float>(-2.0 * cosine / leading),
        .b2 = static_cast<float>((1.0 - (width * reach)) / leading),
        .a1 = static_cast<float>(-2.0 * cosine / leading),
        .a2 = static_cast<float>((1.0 - (width / reach)) / leading),
    };
}

void Biquad::configure(BiquadCoefficients coefficients, std::size_t num_channels)
{
    coefficients_ = coefficients;
    num_channels_ = num_channels;

    // Cleared, not kept: what was being carried belonged to a stream that has ended.
    carried_.assign(num_channels * 2, 0.0F);
}

void Biquad::retune(BiquadCoefficients coefficients) noexcept
{
    coefficients_ = coefficients;
}

void Biquad::process(std::span<float> samples) noexcept
{
    if (num_channels_ == 0)
        return;

    const BiquadCoefficients used{coefficients_};

    // Frame by frame, so that which channel a sample belongs to is counted rather than divided
    // for.
    for (std::size_t frame = 0; frame + num_channels_ <= samples.size(); frame += num_channels_) {
        for (std::size_t channel = 0; channel < num_channels_; ++channel) {
            float& first{carried_[channel * 2]};
            float& second{carried_[(channel * 2) + 1]};

            const float in{samples[frame + channel]};
            const float out{(used.b0 * in) + first};

            first = (used.b1 * in) - (used.a1 * out) + second;
            second = (used.b2 * in) - (used.a2 * out);

            samples[frame + channel] = out;
        }
    }
}

} // namespace wiola::audio
