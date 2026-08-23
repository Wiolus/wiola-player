/**
 * @file
 * @brief What is done to samples between the buffer and the output.
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

#include <audio/chain.hpp>
#include <audio/tuning.hpp>

#include <miniaudio.h>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <span>
#include <vector>

namespace wiola::audio {

/// The bands and what has been asked of them. A gain is kept for every band the layout names, so
/// a format that cannot carry one does not lose its setting.
struct Chain::Bands {
    std::vector<std::atomic<float>> gains;
    std::vector<ma_peak2> filters;
    std::vector<std::vector<unsigned char>> states;
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

ma_peak2_config band_config(StreamSpec spec, double quality, units::Frequency center,
    float gain_db) noexcept
{
    return ma_peak2_config_init(ma_format_f32, static_cast<ma_uint32>(spec.num_channels),
        static_cast<ma_uint32>(spec.sample_rate.get<units::Hz>()), static_cast<double>(gain_db),
        quality, center.get<units::Hz>());
}

} // namespace

Chain::Chain(StreamSpec spec, BandLayout layout)
    : layout_{layout}
    , bands_{std::make_unique<Bands>()}
{
    bands_->gains = std::vector<std::atomic<float>>(layout_.count);

    configure(spec);
}

Chain::~Chain() = default;

void Chain::configure(StreamSpec spec)
{
    spec_ = spec;
    num_bands_ = 0;

    const units::Frequency highest{spec.sample_rate * tuning::highest_band_share};

    // The series ascends, so the first center that does not fit ends it.
    while (num_bands_ < layout_.count && layout_.center(num_bands_) <= highest)
        ++num_bands_;

    bands_->filters.assign(num_bands_, ma_peak2{});
    bands_->states.clear();

    for (std::size_t i = 0; i < num_bands_; ++i) {
        const ma_peak2_config config{band_config(spec_, layout_.quality, band_center(i), 0.0F)};
        std::size_t size{0};

        ma_peak2_get_heap_size(&config, &size);
        bands_->states.emplace_back(std::max<std::size_t>(size, 1));

        // A band that will not initialize ends the row, since the ones above it were built for a
        // format this one has just refused.
        if (ma_peak2_init_preallocated(&config, bands_->states.back().data(),
                &bands_->filters[i]) != MA_SUCCESS) {
            num_bands_ = i;
            break;
        }
    }

    retune();
}

void Chain::retune() noexcept
{
    bands_->preamp = amplitude(preamp());
    bands_->shaping = bands_->preamp != 1.0F;

    for (std::size_t i = 0; i < num_bands_; ++i) {
        const ma_peak2_config config{
            band_config(spec_, layout_.quality, band_center(i), band_gain(i))};

        ma_peak2_reinit(&config, &bands_->filters[i]);

        bands_->shaping = bands_->shaping || band_gain(i) != 0.0F;
    }
}

StreamSpec Chain::spec() const noexcept
{
    return spec_;
}

std::size_t Chain::num_bands() const noexcept
{
    return num_bands_;
}

units::Frequency Chain::band_center(std::size_t index) const noexcept
{
    return layout_.center(index);
}

void Chain::set_band_gain(std::size_t index, float db) noexcept
{
    if (index >= bands_->gains.size())
        return;

    const auto limit = static_cast<float>(tuning::max_band_gain_db);

    bands_->gains[index].store(std::clamp(db, -limit, limit), std::memory_order_relaxed);
    bands_->changed.store(true, std::memory_order_release);
}

float Chain::band_gain(std::size_t index) const noexcept
{
    if (index >= bands_->gains.size())
        return 0.0F;

    return bands_->gains[index].load(std::memory_order_relaxed);
}

void Chain::process(std::span<float> samples) noexcept
{
    // Coefficients are arithmetic, so the thread that plays can afford to work them out itself.
    if (bands_->changed.exchange(false, std::memory_order_acquire))
        retune();

    const bool shaping{bands_->shaping && bands_->enabled.load(std::memory_order_relaxed)};

    if (shaping) {
        apply_gain(samples, bands_->preamp);

        const auto num_frames = static_cast<ma_uint64>(spec_.frames_per(samples.size()));

        for (ma_peak2& filter : bands_->filters)
            ma_peak2_process_pcm_frames(&filter, samples.data(), samples.data(), num_frames);
    }

    const float gain{volume_.load(std::memory_order_relaxed)};

    // Not for speed: at full volume the decoded samples pass through untouched.
    if (gain != 1.0F)
        apply_gain(samples, gain);

    // A band is the only thing here that can lift a sample past what an output takes.
    if (!shaping)
        return;

    for (float& sample : samples)
        sample = std::clamp(sample, -1.0F, 1.0F);
}

void Chain::set_preamp(float db) noexcept
{
    const auto limit = static_cast<float>(tuning::max_preamp_db);

    bands_->preamp_db.store(std::clamp(db, -limit, limit), std::memory_order_relaxed);
    bands_->changed.store(true, std::memory_order_release);
}

float Chain::preamp() const noexcept
{
    return bands_->preamp_db.load(std::memory_order_relaxed);
}

void Chain::set_equalizer_enabled(bool enabled) noexcept
{
    bands_->enabled.store(enabled, std::memory_order_relaxed);
}

bool Chain::equalizer_enabled() const noexcept
{
    return bands_->enabled.load(std::memory_order_relaxed);
}

void Chain::set_volume(float gain) noexcept
{
    // A bound rather than a clamp, so that a value that is not a number falls to silence.
    const float bounded{gain >= 0.0F ? std::min(gain, 1.0F) : 0.0F};

    volume_.store(bounded, std::memory_order_relaxed);
}

float Chain::volume() const noexcept
{
    return volume_.load(std::memory_order_relaxed);
}

} // namespace wiola::audio
