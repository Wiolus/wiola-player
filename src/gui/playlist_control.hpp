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

#pragma once

#include <engine/session.hpp>

#include <QSettings>

#include <cstddef>

namespace wiola::gui {

/**
 * What was queued when a run ended, put back when the next one begins.
 *
 * A listener who queues twenty tracks and closes the window has not asked to do it again. What
 * is kept is the list, the track it stood at, and how it was being played through - not where in
 * that track playback had reached, which is a different thing to promise.
 */
class PlaylistControl {
public:
    PlaylistControl(engine::Session& session, QSettings& settings) noexcept;

    /// Puts back what the last run left. Tracks that have gone since are dropped rather than
    /// queued to fail, and how many were dropped is what this answers.
    std::size_t restore();

    /// Keeps what is queued now, and how it is being played through.
    void save();

private:
    engine::Session& session_;
    QSettings& settings_;
};

} // namespace wiola::gui
