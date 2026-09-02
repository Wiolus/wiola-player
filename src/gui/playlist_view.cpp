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

#include <QFont>
#include <QHeaderView>
#include <QLabel>
#include <QPainter>
#include <QString>
#include <QStyledItemDelegate>

#include <algorithm>
#include <vector>

namespace wiola::gui {

namespace {

/// Which column shows what.
enum Column {
    cover,
    title,
    artist,
    album,
    length,
    num_columns,
};

/// Draws a row the way a table does, without the box a cell wears for having been the last one
/// clicked: a queue is a list of tracks, not a grid of cells to pick out one at a time.
class WithoutTheFocusBox final : public QStyledItemDelegate {
public:
    using QStyledItemDelegate::QStyledItemDelegate;

    void paint(QPainter* painter, const QStyleOptionViewItem& option,
        const QModelIndex& index) const override
    {
        QStyleOptionViewItem without{option};
        without.state &= ~QStyle::State_HasFocus;

        QStyledItemDelegate::paint(painter, without, index);
    }
};

/// `time` as minutes and seconds, or nothing at all when it is not known.
[[nodiscard]] QString as_clock(units::Time time)
{
    if (time == units::Time{})
        return QString{};

    const auto seconds = static_cast<int>(time.get<units::Sec>());

    return QString{"%1:%2"}.arg(seconds / 60).arg(seconds % 60, 2, 10, QChar{'0'});
}

} // namespace

PlaylistView::PlaylistView(QWidget* parent)
    : QTableWidget{parent}
    , covers_{*this, tuning::cover_size}
{
    setColumnCount(Column::num_columns);
    setHorizontalHeaderLabels({QString{}, "Title", "Artist", "Album", "Length"});

    setMinimumHeight(tuning::playlist_height);
    setSelectionMode(QAbstractItemView::ExtendedSelection);
    setSelectionBehavior(QAbstractItemView::SelectRows);
    setEditTriggers(QAbstractItemView::NoEditTriggers);
    setShowGrid(false);
    setWordWrap(false);
    setTextElideMode(Qt::ElideRight);
    setToolTip("Double-click a track to play it; Delete takes it out of the queue");

    verticalHeader()->setVisible(false);
    verticalHeader()->setDefaultSectionSize(tuning::cover_size + tuning::row_padding);

    horizontalHeader()->setSectionResizeMode(Column::cover, QHeaderView::Fixed);
    horizontalHeader()->setSectionResizeMode(Column::title, QHeaderView::Stretch);
    horizontalHeader()->setSectionResizeMode(Column::artist, QHeaderView::Interactive);
    horizontalHeader()->setSectionResizeMode(Column::album, QHeaderView::Interactive);
    horizontalHeader()->setSectionResizeMode(Column::length, QHeaderView::Fixed);

    setColumnWidth(Column::cover, tuning::cover_size + tuning::row_padding);
    setColumnWidth(Column::artist, tuning::artist_column_width);
    setColumnWidth(Column::album, tuning::album_column_width);
    setColumnWidth(Column::length, tuning::length_column_width);

    setItemDelegate(new WithoutTheFocusBox{this});

    connect(this, &QTableWidget::cellActivated,
        [this](int row, int /*column*/) { emit track_chosen(static_cast<std::size_t>(row)); });
}

void PlaylistView::show_playlist(const engine::Playlist& playlist)
{
    const std::size_t revision{playlist.revision()};
    const std::optional<std::size_t> current{playlist.position()};

    if (revision_shown_ == revision && current_shown_ == current)
        return;

    revision_shown_ = revision;
    current_shown_ = current;

    covers_.begin_turn();
    setRowCount(static_cast<int>(playlist.tracks().size()));

    for (std::size_t index = 0; index < playlist.tracks().size(); ++index)
        show_track(static_cast<int>(index), playlist.tracks()[index], current == index);

    // Some pictures were left for another turn, so there has to be another turn: without this a
    // queue longer than a few tracks would keep its stubs until something else changed it.
    if (covers_.ran_out())
        revision_shown_.reset();
}

void PlaylistView::show_track(int at, const engine::Track& track, bool playing)
{
    auto* picture = new QLabel{this};
    picture->setPixmap(covers_.cover_of(track.path));
    picture->setAlignment(Qt::AlignCenter);

    setCellWidget(at, Column::cover, picture);
    setItem(at, Column::title, new QTableWidgetItem{QString::fromStdString(track.title)});
    setItem(at, Column::artist, new QTableWidgetItem{QString::fromStdString(track.artist)});
    setItem(at, Column::album, new QTableWidgetItem{QString::fromStdString(track.album)});
    setItem(at, Column::length, new QTableWidgetItem{as_clock(track.duration)});

    if (!playing)
        return;

    // What is playing is marked, not selected: which rows are picked out is the listener's to
    // say, and a program that keeps choosing for them is a program they are fighting.
    QColor marked{palette().highlight().color()};
    marked.setAlpha(tuning::playing_row_tint);

    QFont bold{font()};
    bold.setBold(true);

    for (int column = Column::title; column < Column::num_columns; ++column) {
        item(at, column)->setFont(bold);
        item(at, column)->setBackground(marked);
    }
}

std::vector<std::size_t> PlaylistView::chosen_rows() const
{
    std::vector<std::size_t> rows;

    for (const QModelIndex& index : selectionModel()->selectedRows())
        rows.push_back(static_cast<std::size_t>(index.row()));

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

    QTableWidget::keyPressEvent(event);
}

} // namespace wiola::gui
