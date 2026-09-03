/**
 * @file
 * @brief The pictures tracks carry, kept small and kept about.
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

#include <QPixmap>
#include <QWidget>

#include <cstddef>
#include <deque>
#include <filesystem>
#include <map>
#include <string>

namespace wiola::gui {

/**
 * The picture each track carries, shrunk to the size a row shows it at and kept for next time.
 *
 * A picture in a tag is often a megabyte and there may be a thousand tracks, so what is kept is
 * what is drawn - a thumbnail - and only for as many tracks as a window is likely to be looking
 * at. What has not been read yet is drawn as a stub, and read on one of the next turns instead:
 * a listener scrolling a long queue never waits for a file.
 */
class CoverCache {
public:
    /// Takes the widget whose style draws the stub, and how many pixels a side a thumbnail is.
    CoverCache(const QWidget& styled, int size);

    /// A turn of drawing begins, and with it the few files this one may read.
    void begin_turn() noexcept;

    /// The picture `path` carries, or the stub while it has not been read.
    [[nodiscard]] const QPixmap& cover_of(const std::filesystem::path& path);

    /// Whether this turn ran out of reads with pictures still to fetch, so that whoever is
    /// drawing knows to come back rather than leaving stubs where covers belong.
    [[nodiscard]] bool ran_out() const noexcept { return ran_out_; }

private:
    /// Forgets the `count` covers read longest ago.
    void forget_oldest(std::size_t count);

    QPixmap stub_;
    int size_;

    std::map<std::string, QPixmap> covers_;

    /// What is held, in the order it was read, so that the oldest are the ones to go.
    std::deque<std::string> order_;
    std::size_t read_this_turn_{0};
    bool ran_out_{false};
};

} // namespace wiola::gui
