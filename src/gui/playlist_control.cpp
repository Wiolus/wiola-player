/**
 * @file
 * @brief The queue a run leaves behind, put back for the next one.
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

#include "playlist_control.hpp"

#include <QString>
#include <QStringList>

#include <filesystem>
#include <utility>
#include <vector>

namespace wiola::gui {

namespace {

/// Under which names the queue is kept.
constexpr auto tracks_key{"queue"};
constexpr auto standing_key{"queue/at"};
constexpr auto repeat_key{"queue/repeat"};
constexpr auto shuffled_key{"queue/shuffled"};

/// `stored` as a repeat, or none when it is not one: what a settings file holds is whatever was
/// last written there, which a newer or older run may not agree about.
engine::Playlist::Repeat repeat_of(int stored) noexcept
{
    using Repeat = engine::Playlist::Repeat;

    switch (stored) {
    case static_cast<int>(Repeat::track):
        return Repeat::track;

    case static_cast<int>(Repeat::all):
        return Repeat::all;

    default:
        return Repeat::none;
    }
}

} // namespace

PlaylistControl::PlaylistControl(engine::Session& session, QSettings& settings) noexcept
    : session_{session}
    , settings_{settings}
{
}

std::size_t PlaylistControl::restore()
{
    const QStringList stored{settings_.value(tracks_key).toStringList()};

    std::vector<std::filesystem::path> tracks;
    std::size_t gone{0};

    for (const QString& name : stored) {
        std::filesystem::path track{name.toStdString()};

        // A queue that cannot be played is worse than a shorter one.
        if (std::filesystem::exists(track))
            tracks.push_back(std::move(track));
        else
            ++gone;
    }

    session_.set_repeat(repeat_of(settings_.value(repeat_key, 0).toInt()));

    if (tracks.empty())
        return gone;

    const auto standing = static_cast<std::size_t>(settings_.value(standing_key, 0).toInt());

    session_.add(std::move(tracks));
    static_cast<void>(session_.play_track(standing));

    // The order a shuffle drew is not kept: what is put back is that a listener asked for one.
    if (settings_.value(shuffled_key, false).toBool())
        session_.shuffle(true);

    return gone;
}

void PlaylistControl::save()
{
    QStringList names;

    // The file is what is kept: what is known about a track is found out again, and may have
    // changed since.
    for (const engine::Track& track : session_.playlist().tracks())
        names.append(QString::fromStdString(track.path.string()));

    settings_.setValue(tracks_key, names);
    settings_.setValue(standing_key, static_cast<int>(session_.playlist().position().value_or(0)));
    settings_.setValue(repeat_key, static_cast<int>(session_.repeat()));
    settings_.setValue(shuffled_key, session_.shuffled());
}

} // namespace wiola::gui
