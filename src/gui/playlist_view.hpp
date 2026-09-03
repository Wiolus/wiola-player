/**
 * @file
 * @brief The queue, as a listener sees it.
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

#include "cover_cache.hpp"

#include <engine/queue/playlist.hpp>

#include <QKeyEvent>
#include <QTableWidget>

#include <cstddef>
#include <optional>
#include <vector>

namespace wiola::gui {

/**
 * The tracks that are queued: what each carries as a picture, what it is called, who it is by,
 * what it is from and how long it runs, with the one being played marked.
 *
 * It is shown rather than kept: what is queued lives in the session, and this is redrawn from it
 * only when it has changed, so that a window refreshing ten times a second does not rebuild a
 * table nobody has touched.
 */
class PlaylistView final : public QTableWidget {
    Q_OBJECT

public:
    explicit PlaylistView(QWidget* parent = nullptr);

    /// Draws `playlist` if what it holds, or where it stands, has moved since the last time.
    void show_playlist(const engine::Playlist& playlist);

    /// Which rows a listener has picked out, from the last towards the first: taking them out in
    /// that order leaves the rows still to go where they were.
    [[nodiscard]] std::vector<std::size_t> chosen_rows() const;

signals:
    /// Asks for the track that was put at `index` to be played.
    void track_chosen(std::size_t index);

    /// Asks for whatever rows are picked out to be taken out of the queue.
    void removal_asked();

protected:
    void keyPressEvent(QKeyEvent* event) override;

private:
    /// Puts `track` in the row at `at`, picture and all, marked when it is the one playing.
    void show_track(int at, const engine::Track& track, bool playing);

    CoverCache covers_;

    /// What is drawn now, so that redrawing is only done when it would show something else.
    std::optional<std::size_t> revision_shown_;
    std::optional<std::size_t> current_shown_;
};

} // namespace wiola::gui
