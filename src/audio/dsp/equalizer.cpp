/**
 * @file
 * @brief Lift and cut, one band at a time.
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

#include <audio/dsp/equalizer.hpp>

#include <audio/dsp/biquad.hpp>
#include <audio/dsp/tuning.hpp>

#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstddef>
#include <span>
#include <vector>

namespace wiola::audio {

/// The bands and what has been asked of them. A gain is kept for every band the layout names, so
/// a format that cannot carry one does not lose its setting.
struct Equalizer::Bands {
    std::vector<std::atomic<float>> gains;
    std::vector<Biquad> filters;
    std::atomic<float> preamp_db{0.0F};
    std::atomic<bool> enabled{true};
    std::atomic<bool> changed{false};

    /// What `retune` left for `process`: the preamp as a factor, and whether anything here would
    /// change a sample.
    float preamp{1.0F};
    bool shaping{false};
};

namespace {

/// `db` as a factor to multiply a sample by.
float amplitude(float db) noexcept
{
    return static_cast<float>(std::pow(10.0, static_cast<double>(db) / 20.0));
}

void apply_gain(std::span<float> samples, float gain) noexcept
{
    for (float& sample : samples)
        sample *= gain;
}

} // namespace

Equalizer::Equalizer(pcm::StreamSpec spec, BandLayout layout)
    : layout_{layout}
    , bands_{std::make_unique<Bands>()}
{
    bands_->gains = std::vector<std::atomic<float>>(layout_.count);

    configure(spec);
}

Equalizer::~Equalizer() = default;

void Equalizer::configure(pcm::StreamSpec spec)
{
    spec_ = spec;
    num_bands_ = 0;

    const units::Frequency highest{spec.sample_rate * tuning::highest_band_share};

    // The series ascends, so the first center that does not fit ends it.
    while (num_bands_ < layout_.count && layout_.center(num_bands_) <= highest)
        ++num_bands_;

    bands_->filters.clear();
    bands_->filters.resize(num_bands_);

    for (std::size_t i = 0; i < num_bands_; ++i)
        bands_->filters[i].configure(BiquadCoefficients::peaking(spec_, band_center(i),
                                         layout_.quality, 0.0),
            spec_.num_channels);

    retune();
}

void Equalizer::retune() noexcept
{
    float lifted{0.0F};

    bands_->shaping = false;

    for (std::size_t i = 0; i < num_bands_; ++i) {
        const float gain{band_gain(i)};

        // Kept, not rebuilt: the sound carries on through a gain the listener has just moved.
        bands_->filters[i].retune(BiquadCoefficients::peaking(spec_, band_center(i),
            layout_.quality, gain));

        lifted = std::max(lifted, gain);
        bands_->shaping = bands_->shaping || gain != 0.0F;
    }

    // The room a boost needs, taken before the bands rather than asked of the listener. Without
    // it a lifted band reaches past what an output takes, and is flattened rather than heard.
    bands_->preamp_db.store(-lifted, std::memory_order_relaxed);
    bands_->preamp = amplitude(-lifted);
}

const BandLayout& Equalizer::layout() const noexcept
{
    return layout_;
}

std::size_t Equalizer::num_bands() const noexcept
{
    return num_bands_;
}

units::Frequency Equalizer::band_center(std::size_t index) const noexcept
{
    return layout_.center(index);
}

void Equalizer::set_band_gain(std::size_t index, float db) noexcept
{
    if (index >= bands_->gains.size())
        return;

    const auto limit = static_cast<float>(tuning::max_band_gain_db);

    bands_->gains[index].store(std::clamp(db, -limit, limit), std::memory_order_relaxed);
    bands_->changed.store(true, std::memory_order_release);
}

float Equalizer::band_gain(std::size_t index) const noexcept
{
    if (index >= bands_->gains.size())
        return 0.0F;

    return bands_->gains[index].load(std::memory_order_relaxed);
}

void Equalizer::process(std::span<float> samples) noexcept
{
    // Coefficients are arithmetic, so the thread that plays can afford to work them out itself.
    if (bands_->changed.exchange(false, std::memory_order_acquire))
        retune();

    if (!bands_->shaping || !bands_->enabled.load(std::memory_order_relaxed))
        return;

    apply_gain(samples, bands_->preamp);

    for (Biquad& filter : bands_->filters)
        filter.process(samples);
}

float Equalizer::preamp() const noexcept
{
    return bands_->preamp_db.load(std::memory_order_relaxed);
}

void Equalizer::set_enabled(bool enabled) noexcept
{
    bands_->enabled.store(enabled, std::memory_order_relaxed);
}

bool Equalizer::enabled() const noexcept
{
    return bands_->enabled.load(std::memory_order_relaxed);
}

} // namespace wiola::audio
