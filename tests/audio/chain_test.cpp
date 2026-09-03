/**
 * @file
 * @brief Tests for what is done to samples on their way to the output.
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

#include <audio/dsp/chain.hpp>

#include <audio/dsp/stage.hpp>
#include <fakes/tone.hpp>
#include <fixtures/shaping.hpp>
#include <pcm/stream_spec.hpp>
#include <utils/units.hpp>

#include <gtest/gtest.h>

#include <array>
#include <cmath>
#include <cstddef>
#include <span>
#include <string>
#include <vector>

namespace {

using wiola::audio::Chain;
using wiola::audio::Stage;
using wiola::pcm::Frames;
using wiola::pcm::StreamSpec;
using namespace wiola::units;

constexpr StreamSpec stereo{.sample_rate = 48_kHz, .num_channels = 2};

auto samples()
{
    return std::array{1.0F, -0.5F, 0.25F, 0.0F};
}

/// A step that says when it ran and multiplies by whatever it was built with.
class Marking final : public Stage {
public:
    Marking(std::vector<std::string>& ran, std::string name, float factor)
        : ran_{ran}
        , name_{std::move(name)}
        , factor_{factor}
    {
    }

    void configure(StreamSpec spec) override
    {
        configured = spec;
        ++num_configures;
    }

    void process(std::span<float> samples) noexcept override
    {
        ran_.push_back(name_);

        for (float& sample : samples)
            sample *= factor_;
    }

    StreamSpec configured{};
    int num_configures{0};

private:
    std::vector<std::string>& ran_;
    std::string name_;
    float factor_;
};

} // namespace

TEST(Chain, LeavesSamplesAloneWithNoSteps)
{
    Chain chain;
    auto buffer{samples()};

    chain.process(buffer);

    EXPECT_EQ(buffer, samples());
}

TEST(Chain, RunsStepsInTheOrderTheyWereAdded)
{
    std::vector<std::string> ran;
    Marking first{ran, "first", 1.0F};
    Marking second{ran, "second", 1.0F};
    Marking third{ran, "third", 1.0F};

    Chain chain;
    chain.add(first);
    chain.add(second);
    chain.add(third);

    auto buffer{samples()};
    chain.process(buffer);

    EXPECT_EQ(ran, (std::vector<std::string>{"first", "second", "third"}));
}

TEST(Chain, RunsEveryStepOverTheSamples)
{
    std::vector<std::string> ran;
    Marking first{ran, "half", 0.5F};
    Marking second{ran, "half again", 0.5F};

    Chain chain;
    chain.add(first);
    chain.add(second);

    std::array block{0.8F, 0.4F};
    chain.process(block);

    EXPECT_FLOAT_EQ(block[0], 0.2F);
    EXPECT_FLOAT_EQ(block[1], 0.1F);
}

TEST(Chain, TellsEveryStepAboutTheFormat)
{
    std::vector<std::string> ran;
    Marking first{ran, "first", 1.0F};
    Marking second{ran, "second", 1.0F};

    Chain chain;
    chain.add(first);
    chain.add(second);

    chain.configure(stereo);

    EXPECT_EQ(first.num_configures, 1);
    EXPECT_EQ(second.num_configures, 1);
    EXPECT_EQ(first.configured, stereo);
    EXPECT_EQ(second.configured, stereo);
}

/// The ceiling is the chain's own, kept after every step whatever the steps did. A step is never
/// asked whether it lifted, so a step that lifts and does not say so cannot hand over more than
/// an output takes.
TEST(Chain, HoldsTheCeilingWhateverTheStepsDid)
{
    std::vector<std::string> ran;
    Marking loud{ran, "loud", 4.0F};

    Chain chain;
    chain.add(loud);

    std::array block{0.8F, -0.8F, 0.1F};
    chain.process(block);

    EXPECT_FLOAT_EQ(block[0], 1.0F);
    EXPECT_FLOAT_EQ(block[1], -1.0F);
    EXPECT_FLOAT_EQ(block[2], 0.4F);
}

/// What arrives past full scale is held to it too, whether or not anything in the chain lifted.
TEST(Chain, HoldsTheCeilingOnWhatArrivedPastIt)
{
    Chain chain;
    std::array block{2.0F, -3.0F, 0.5F};

    chain.process(block);

    EXPECT_FLOAT_EQ(block[0], 1.0F);
    EXPECT_FLOAT_EQ(block[1], -1.0F);
    EXPECT_FLOAT_EQ(block[2], 0.5F);
}

TEST(Chain, AcceptsNothingToDo)
{
    std::vector<std::string> ran;
    Marking half{ran, "half", 0.5F};

    Chain chain;
    chain.add(half);
    chain.process(std::span<float>{});

    // Nothing to do is not something to recover from: what comes after is shaped as it would
    // have been.
    std::array block{1.0F, 1.0F};
    chain.process(block);

    EXPECT_FLOAT_EQ(block[0], 0.5F);
    EXPECT_FLOAT_EQ(block[1], 0.5F);
}

/// The bands first, then how loud: what a listener asks for last is the last thing done.
TEST(Chain, ShapesThenSetsHowLoud)
{
    wiola::testing::Shaping shaping{stereo};
    auto buffer{samples()};

    shaping.equalizer.set_band_gain(5, 12.0F);
    shaping.equalizer.set_enabled(false);
    shaping.volume.set_gain(0.5F);
    shaping.chain.process(buffer);

    EXPECT_FLOAT_EQ(buffer[0], 0.5F);
    EXPECT_FLOAT_EQ(buffer[1], -0.25F);
}

TEST(Chain, TakesANewFormatWithoutForgettingTheSetting)
{
    wiola::testing::Shaping shaping{stereo};

    shaping.volume.set_gain(0.5F);
    shaping.chain.configure(StreamSpec{.sample_rate = 22050_Hz, .num_channels = 1});

    EXPECT_FLOAT_EQ(shaping.volume.gain(), 0.5F);
    EXPECT_EQ(shaping.equalizer.num_bands(), 9u);

    shaping.chain.configure(stereo);

    EXPECT_EQ(shaping.equalizer.num_bands(), 10u);
}

/// A boosted band can ask for more than an output takes, and is not allowed to hand it over.
TEST(Chain, KeepsBoostedSamplesInRange)
{
    wiola::testing::Shaping shaping{stereo};
    wiola::testing::SineSource source{stereo, 1_kHz, 0.95F};
    std::vector<float> block(stereo.samples_per(Frames{512}));

    shaping.equalizer.set_band_gain(5, 12.0F);
    shaping.volume.set_gain(wiola::audio::tuning::max_volume_boost);

    for (int i = 0; i < 20; ++i) {
        source.render(block);
        shaping.chain.process(block);

        for (const float sample : block)
            ASSERT_LE(std::abs(sample), 1.0F);
    }
}
