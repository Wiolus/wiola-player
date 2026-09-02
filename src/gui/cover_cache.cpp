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

#include "cover_cache.hpp"

#include "tuning.hpp"

#include <codec/tags.hpp>

#include <QStyle>

namespace wiola::gui {

CoverCache::CoverCache(const QWidget& styled, int size)
    : stub_{styled.style()->standardIcon(QStyle::SP_FileIcon).pixmap(size, size)}
    , size_{size}
{
}

void CoverCache::begin_turn() noexcept
{
    read_this_turn_ = 0;
    ran_out_ = false;
}

const QPixmap& CoverCache::cover_of(const std::filesystem::path& path)
{
    const std::string key{path.string()};

    if (const auto found = covers_.find(key); found != covers_.end())
        return found->second;

    // The rest of the queue waits for another turn: a window is drawn ten times a second, so a
    // long queue fills in within a moment without any one turn stopping to read files.
    if (read_this_turn_ >= tuning::covers_read_per_turn) {
        ran_out_ = true;

        return stub_;
    }

    ++read_this_turn_;

    // Kept from growing without end: a queue longer than this is one whose far end is not being
    // looked at.
    if (covers_.size() >= tuning::covers_kept)
        covers_.clear();

    const codec::Tags tags{codec::read_tags(path)};
    QPixmap art;

    if (!tags.art.empty())
        art.loadFromData(reinterpret_cast<const uchar*>(tags.art.data()),
            static_cast<unsigned int>(tags.art.size()));

    covers_.emplace(key,
        art.isNull() ? stub_
                     : art.scaled(size_, size_, Qt::KeepAspectRatio, Qt::SmoothTransformation));

    return covers_[key];
}

} // namespace wiola::gui
