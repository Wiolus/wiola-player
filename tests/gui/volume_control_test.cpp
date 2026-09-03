/**
 * @file
 * @brief Unit tests for the volume control.
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

#include <gui/tuning.hpp>
#include <gui/volume_control.hpp>

#include <audio/dsp/volume.hpp>

#include <QSettings>
#include <QString>

#include <gtest/gtest.h>

#include <filesystem>

namespace {

using wiola::audio::Volume;
using Control = wiola::gui::VolumeControl;
namespace tuning = wiola::gui::tuning;

/// A settings file of its own, so a test neither reads nor leaves anything of the user's.
class VolumeControl : public testing::Test {
protected:
    static void SetUpTestSuite()
    {
        QSettings::setPath(QSettings::IniFormat, QSettings::UserScope,
            QString::fromStdString(std::filesystem::temp_directory_path().string()));
    }

    void SetUp() override { settings.clear(); }

    QSettings settings{QSettings::IniFormat, QSettings::UserScope, "wiola-test", "volume"};
    Volume volume;
};

TEST_F(VolumeControl, StartsAtFullVolume)
{
    Control control{volume, settings};

    EXPECT_EQ(control.restore(), tuning::full_volume);
    EXPECT_EQ(control.position(), tuning::full_volume);
    EXPECT_FLOAT_EQ(volume.gain(), 1.0F);
}

/// The curve is the point of the control: half the travel is a quarter of the gain, not half.
TEST_F(VolumeControl, AppliesTheCurveToAPosition)
{
    Control control{volume, settings};

    control.set_position(50);

    EXPECT_EQ(control.position(), 50);
    EXPECT_FLOAT_EQ(volume.gain(), 0.25F);
}

TEST_F(VolumeControl, IsSilentAtTheBottomOfTheTravel)
{
    Control control{volume, settings};

    control.set_position(0);

    EXPECT_FLOAT_EQ(volume.gain(), 0.0F);
}

TEST_F(VolumeControl, ClampsAPositionOffTheTravel)
{
    Control control{volume, settings};

    control.set_position(-5);
    EXPECT_EQ(control.position(), 0);
    EXPECT_FLOAT_EQ(volume.gain(), 0.0F);

    control.set_position(500);
    EXPECT_EQ(control.position(), tuning::full_volume);
    EXPECT_FLOAT_EQ(volume.gain(), 1.0F);
}

/// What the control is for: a run leaves its position for the next one.
TEST_F(VolumeControl, GivesBackWhatTheLastRunLeft)
{
    Control{volume, settings}.set_position(70);

    Volume next;
    Control later{next, settings};

    EXPECT_EQ(later.restore(), 70);
    EXPECT_FLOAT_EQ(next.gain(), 0.49F);
}

/// Restoring is applying, not only reading: the volume it was built on is set by it.
TEST_F(VolumeControl, AppliesWhatItRestores)
{
    Control{volume, settings}.set_position(50);

    Volume next;

    EXPECT_FLOAT_EQ(next.gain(), 1.0F);

    Control{next, settings}.restore();

    EXPECT_FLOAT_EQ(next.gain(), 0.25F);
}

/// The file is text and can be edited, and a typo is not a reason to start silent.
TEST_F(VolumeControl, IgnoresAStoredValueThatIsNotAPosition)
{
    settings.setValue("volume", "loud");

    Control control{volume, settings};

    EXPECT_EQ(control.restore(), tuning::full_volume);
}

TEST_F(VolumeControl, BringsAStoredPositionBackIntoRange)
{
    settings.setValue("volume", 9999);

    EXPECT_EQ(Control(volume, settings).restore(), tuning::full_volume);

    settings.setValue("volume", -20);

    EXPECT_EQ(Control(volume, settings).restore(), 0);
}

/// Full volume is where the slider stops until the listener asks for more.
TEST_F(VolumeControl, StopsAtFullUntilBoosted)
{
    Control control{volume, settings};

    EXPECT_FALSE(control.boosted());
    EXPECT_EQ(control.max_position(), tuning::full_volume);

    control.set_position(130);

    EXPECT_EQ(control.position(), tuning::full_volume);
    EXPECT_FLOAT_EQ(volume.gain(), 1.0F);
}

/// Past full the slider says what it means: a hundred and forty is what arrived and forty more.
TEST_F(VolumeControl, LiftsPastWhatArrivedWhenBoosted)
{
    Control control{volume, settings};

    control.set_boosted(true);

    EXPECT_EQ(control.max_position(), tuning::boosted_volume);

    control.set_position(tuning::boosted_volume);

    EXPECT_EQ(control.position(), tuning::boosted_volume);
    EXPECT_FLOAT_EQ(volume.gain(), 1.4F);
}

/// The curve is below full only, so full is full either way.
TEST_F(VolumeControl, LeavesTheTravelBelowFullAlone)
{
    Control control{volume, settings};

    control.set_boosted(true);
    control.set_position(50);

    EXPECT_FLOAT_EQ(volume.gain(), 0.25F);

    control.set_position(tuning::full_volume);

    EXPECT_FLOAT_EQ(volume.gain(), 1.0F);
}

/// Turning it off is not a way to keep a position the slider can no longer reach.
TEST_F(VolumeControl, BringsAPositionBackWhenBoostIsTurnedOff)
{
    Control control{volume, settings};

    control.set_boosted(true);
    control.set_position(130);
    control.set_boosted(false);

    EXPECT_EQ(control.position(), tuning::full_volume);
    EXPECT_FLOAT_EQ(volume.gain(), 1.0F);
}

TEST_F(VolumeControl, GivesBackWhetherItWasBoosted)
{
    Control first{volume, settings};

    first.set_boosted(true);
    first.set_position(120);

    Volume next;
    Control later{next, settings};

    EXPECT_EQ(later.restore(), 120);
    EXPECT_TRUE(later.boosted());
    EXPECT_FLOAT_EQ(next.gain(), 1.2F);
}

} // namespace
