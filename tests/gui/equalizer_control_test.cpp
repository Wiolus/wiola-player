/**
 * @file
 * @brief Unit tests for the equalizer control.
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
#include <audio/dsp/tuning.hpp>
#include <gui/equalizer_control.hpp>
#include <pcm/stream_spec.hpp>
#include <utils/units.hpp>

#include <QSettings>
#include <QString>

#include <gtest/gtest.h>

#include <cstddef>
#include <filesystem>

namespace {

using namespace wiola::units::literals;
using Control = wiola::gui::EqualizerControl;
using wiola::audio::Equalizer;
using wiola::pcm::StreamSpec;

constexpr StreamSpec stereo{.sample_rate = 48_kHz, .num_channels = 2};

/// A settings file of its own, so a test neither reads nor leaves anything of the user's.
class EqualizerControl : public testing::Test {
protected:
    static void SetUpTestSuite()
    {
        QSettings::setPath(QSettings::IniFormat, QSettings::UserScope,
            QString::fromStdString(std::filesystem::temp_directory_path().string()));
    }

    void SetUp() override { settings.clear(); }

    QSettings settings{QSettings::IniFormat, QSettings::UserScope, "wiola-test", "equalizer"};
    Equalizer equalizer{stereo};
};

TEST_F(EqualizerControl, StartsFlatAndOn)
{
    Control control{equalizer, settings};

    control.restore();

    EXPECT_TRUE(control.enabled());
    EXPECT_FLOAT_EQ(control.preamp(), 0.0F);

    for (std::size_t index = 0; index < control.num_bands(); ++index)
        EXPECT_FLOAT_EQ(control.band_gain(index), 0.0F) << "band " << index;
}

TEST_F(EqualizerControl, GivesBackWhatTheLastRunLeft)
{
    Control{equalizer, settings}.set_band_gain(2, 6.0F);

    Equalizer next{stereo};
    Control later{next, settings};

    later.restore();

    EXPECT_FLOAT_EQ(later.band_gain(2), 6.0F);
}

TEST_F(EqualizerControl, KeepsWhetherItRuns)
{
    Control control{equalizer, settings};

    control.set_enabled(false);

    Equalizer next{stereo};
    Control later{next, settings};

    later.restore();

    EXPECT_FALSE(later.enabled());
}

/// What is stored is what was applied: the equalizer decides what it allows, not the file.
TEST_F(EqualizerControl, KeepsWhatTheEqualizerAllowed)
{
    Control control{equalizer, settings};
    const auto limit = static_cast<float>(wiola::audio::tuning::max_band_gain_db);

    control.set_band_gain(0, 99.0F);

    EXPECT_FLOAT_EQ(control.band_gain(0), limit);

    Equalizer next{stereo};
    Control later{next, settings};

    later.restore();

    EXPECT_FLOAT_EQ(later.band_gain(0), limit);
}

/// A band this format cannot carry still has a setting, so it is there for the track that can.
TEST_F(EqualizerControl, KeepsABandTheFormatCannotCarry)
{
    Equalizer narrow{
        StreamSpec{.sample_rate = 8_kHz, .num_channels = 2}
    };
    Control control{narrow, settings};

    const std::size_t beyond{narrow.num_bands()};
    ASSERT_LT(beyond, narrow.layout().count);

    control.set_band_gain(beyond, 5.0F);

    Equalizer wide{stereo};
    Control later{wide, settings};

    later.restore();

    ASSERT_GT(wide.num_bands(), beyond);
    EXPECT_FLOAT_EQ(later.band_gain(beyond), 5.0F);
}

/// The file is text and can be edited, and a typo leaves the band flat rather than anywhere.
TEST_F(EqualizerControl, IgnoresAStoredValueThatIsNotAGain)
{
    settings.setValue("equalizer/band/1", "high");

    Control control{equalizer, settings};

    control.restore();

    EXPECT_FLOAT_EQ(control.band_gain(1), 0.0F);
}

TEST_F(EqualizerControl, RestoresEveryBandTheLayoutNames)
{
    Control control{equalizer, settings};

    for (std::size_t index = 0; index < equalizer.layout().count; ++index)
        control.set_band_gain(index, static_cast<float>(index) - 5.0F);

    Equalizer next{stereo};
    Control later{next, settings};

    later.restore();

    for (std::size_t index = 0; index < equalizer.layout().count; ++index)
        EXPECT_FLOAT_EQ(later.band_gain(index), static_cast<float>(index) - 5.0F)
            << "band " << index;
}

} // namespace
