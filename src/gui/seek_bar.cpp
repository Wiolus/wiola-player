/**
 * @file
 * @brief The bar a listener moves through a track with.
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

#include "seek_bar.hpp"

#include "tuning.hpp"

#include <QMouseEvent>
#include <QPainter>

#include <algorithm>

namespace wiola::gui {

SeekBar::SeekBar(QWidget* parent)
    : QWidget{parent}
{
    setFixedHeight(tuning::seek_bar_height);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    setCursor(Qt::PointingHandCursor);
}

void SeekBar::set_fraction(double fraction)
{
    const double shown{std::clamp(fraction, 0.0, 1.0)};

    if (dragging_ || shown == fraction_)
        return;

    fraction_ = shown;
    update();
}

double SeekBar::fraction_at(int x) const noexcept
{
    if (width() <= 0)
        return 0.0;

    return std::clamp(static_cast<double>(x) / width(), 0.0, 1.0);
}

void SeekBar::paintEvent(QPaintEvent* /*event*/)
{
    QPainter painter{this};

    const QRect track{rect()};
    const auto played{static_cast<int>(fraction_ * track.width())};
    const QPalette& colors{palette()};

    painter.fillRect(track, colors.color(QPalette::Mid));
    painter.fillRect(QRect{track.left(), track.top(), played, track.height()},
        colors.color(isEnabled() ? QPalette::Highlight : QPalette::Dark));

    // The edge of the filled part is the position, and is what a drag takes hold of, so it is
    // drawn rather than left as wherever two colors happen to meet.
    if (isEnabled()) {
        painter.fillRect(QRect{track.left() + played - tuning::seek_bar_edge_width / 2, track.top(),
                             tuning::seek_bar_edge_width, track.height()},
            colors.color(QPalette::WindowText));
    }
}

void SeekBar::mousePressEvent(QMouseEvent* event)
{
    if (event->button() != Qt::LeftButton) {
        QWidget::mousePressEvent(event);
        return;
    }

    dragging_ = true;
    fraction_ = fraction_at(event->position().toPoint().x());
    update();

    emit dragged();
}

void SeekBar::mouseMoveEvent(QMouseEvent* event)
{
    if (!dragging_) {
        QWidget::mouseMoveEvent(event);
        return;
    }

    fraction_ = fraction_at(event->position().toPoint().x());
    update();

    emit dragged();
}

void SeekBar::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() != Qt::LeftButton || !dragging_) {
        QWidget::mouseReleaseEvent(event);
        return;
    }

    dragging_ = false;

    emit seek_requested(fraction_);
}

} // namespace wiola::gui
