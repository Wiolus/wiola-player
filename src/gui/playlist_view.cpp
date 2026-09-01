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

#include "playlist_view.hpp"

#include "tuning.hpp"

#include <QString>

#include <algorithm>

#include <filesystem>
#include <vector>

namespace wiola::gui {

PlaylistView::PlaylistView(QWidget* parent)
    : QListWidget{parent}
{
    setMinimumHeight(tuning::playlist_height);
    setSelectionMode(QAbstractItemView::ExtendedSelection);
    setToolTip("Double-click a track to play it; Delete takes it out of the queue");

    connect(this, &QListWidget::itemActivated, this,
        [this](QListWidgetItem* item) { emit track_chosen(static_cast<std::size_t>(row(item))); });
}

std::vector<std::size_t> PlaylistView::chosen_rows() const
{
    std::vector<std::size_t> rows;

    for (const QListWidgetItem* item : selectedItems())
        rows.push_back(static_cast<std::size_t>(row(item)));

    // Last first, so that taking one out does not move the ones still to go.
    std::ranges::sort(rows, std::ranges::greater{});

    return rows;
}

void PlaylistView::keyPressEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Delete) {
        emit removal_asked();

        return;
    }

    QListWidget::keyPressEvent(event);
}

void PlaylistView::show_playlist(const engine::Playlist& playlist)
{
    const std::vector<engine::Track>& tracks{playlist.tracks()};
    const std::size_t current{playlist.position().value_or(0)};

    const bool same{anything_shown_ && num_shown_ == tracks.size() && current_shown_ == current};

    if (same)
        return;

    num_shown_ = tracks.size();
    current_shown_ = current;
    anything_shown_ = true;

    clear();

    for (const engine::Track& track : tracks)
        addItem(QString::fromStdString(track.title));

    if (!tracks.empty())
        setCurrentRow(static_cast<int>(current));
}

} // namespace wiola::gui
