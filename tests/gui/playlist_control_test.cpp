/**
 * @file
 * @brief Unit tests for the queue a run leaves behind.
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

#include <gui/playlist_control.hpp>

#include <engine/session.hpp>

#include <fakes/output.hpp>
#include <fixtures/wav.hpp>

#include <QSettings>
#include <QString>

#include <gtest/gtest.h>

#include <chrono>
#include <filesystem>
#include <memory>
#include <thread>

namespace {

using Repeat = wiola::engine::Playlist::Repeat;
using Control = wiola::gui::PlaylistControl;

/// A settings file of its own, so a test neither reads nor leaves anything of the user's, and a
/// session that plays nowhere real.
class PlaylistControl : public testing::Test {
protected:
    static void SetUpTestSuite()
    {
        QSettings::setPath(QSettings::IniFormat, QSettings::UserScope,
            QString::fromStdString(std::filesystem::temp_directory_path().string()));
    }

    void SetUp() override { settings.clear(); }

    /// Turns the session over until it has caught up with what was asked of it.
    static void settle(wiola::engine::Session& session)
    {
        const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds{5};

        while (session.reading() && std::chrono::steady_clock::now() < deadline)
            std::this_thread::yield();

        session.catch_up();
    }

    static wiola::engine::OutputFactory nowhere()
    {
        return [](wiola::audio::Source& /*source*/) {
            return std::make_unique<wiola::testing::FakeOutput>();
        };
    }

    QSettings settings{QSettings::IniFormat, QSettings::UserScope, "wiola-test", "queue"};
};

} // namespace

TEST_F(PlaylistControl, RestoresNothingFromAnEmptySettingsFile)
{
    wiola::engine::Session session{nowhere()};
    Control control{session, settings};

    EXPECT_EQ(control.restore(), 0U);
    EXPECT_TRUE(session.playlist().empty());
}

TEST_F(PlaylistControl, PutsBackTheQueueTheLastRunLeft)
{
    const std::filesystem::path first{wiola::testing::write_wav("wiola_queue.wav")};
    const std::filesystem::path next{wiola::testing::write_wav("wiola_queue_next.wav")};

    {
        wiola::engine::Session session{nowhere()};
        Control control{session, settings};

        session.add({first, next});
        settle(session);
        control.save();
    }

    wiola::engine::Session restored{nowhere()};
    Control control{restored, settings};

    EXPECT_EQ(control.restore(), 0U);

    ASSERT_EQ(restored.playlist().tracks().size(), 2U);
    EXPECT_EQ(restored.playlist().tracks().front().path, first);
    EXPECT_EQ(restored.playlist().tracks().back().path, next);
}

/// It stands where it stood, without playing: putting a queue back is not pressing play.
TEST_F(PlaylistControl, StandsWhereTheLastRunStood)
{
    const std::filesystem::path first{wiola::testing::write_wav("wiola_queue.wav")};
    const std::filesystem::path next{wiola::testing::write_wav("wiola_queue_next.wav")};

    {
        wiola::engine::Session session{nowhere()};
        Control control{session, settings};

        session.add({first, next});
        settle(session);
        ASSERT_TRUE(session.play_track(1));
        settle(session);
        control.save();
    }

    wiola::engine::Session restored{nowhere()};
    Control control{restored, settings};

    ASSERT_EQ(control.restore(), 0U);
    settle(restored);

    EXPECT_EQ(restored.playlist().position(), 1U);
    EXPECT_EQ(restored.track(), next);
    EXPECT_FALSE(restored.playing());
}

TEST_F(PlaylistControl, PutsBackHowItWasBeingPlayedThrough)
{
    {
        wiola::engine::Session session{nowhere()};
        Control control{session, settings};

        session.add({wiola::testing::write_wav("wiola_queue.wav")});
        settle(session);
        session.set_repeat(Repeat::track);
        session.shuffle(true);
        control.save();
    }

    wiola::engine::Session restored{nowhere()};
    Control control{restored, settings};

    ASSERT_EQ(control.restore(), 0U);

    EXPECT_EQ(restored.repeat(), Repeat::track);
    EXPECT_TRUE(restored.shuffled());
}

/// A queue that cannot be played is worse than a shorter one.
TEST_F(PlaylistControl, DropsTracksThatHaveGoneSince)
{
    const std::filesystem::path kept{wiola::testing::write_wav("wiola_queue.wav")};
    const std::filesystem::path gone{wiola::testing::write_wav("wiola_queue_gone.wav")};

    {
        wiola::engine::Session session{nowhere()};
        Control control{session, settings};

        session.add({kept, gone});
        settle(session);
        control.save();
    }

    std::filesystem::remove(gone);

    wiola::engine::Session restored{nowhere()};
    Control control{restored, settings};

    EXPECT_EQ(control.restore(), 1U) << "the track that had gone was not counted";

    ASSERT_EQ(restored.playlist().tracks().size(), 1U);
    EXPECT_EQ(restored.playlist().tracks().front().path, kept);
}
