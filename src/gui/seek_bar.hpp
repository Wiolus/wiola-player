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

#pragma once

#include <QWidget>

namespace wiola::gui {

/**
 * How far through a track playback is, as a filled part of a rectangle.
 *
 * The whole bar is the track and the filled part is what has been played, so where the two meet
 * is the position. Pressing anywhere moves that meeting point to the press, and it follows the
 * mouse until it is let go, which is when a move is asked for. Nothing is asked for during the
 * drag: each one would stop and start the output.
 *
 * A fraction of the whole is all this deals in. It knows no seconds and no frames, so what a
 * fraction means is settled by whoever owns the track.
 */
class SeekBar final : public QWidget {
    Q_OBJECT

public:
    explicit SeekBar(QWidget* parent = nullptr);

    /// Shows `fraction` of the bar as played. Ignored while it is being dragged, so the bar does
    /// not fight the hand holding it.
    void set_fraction(double fraction);

    /// Where the played edge sits, from 0 at the start of the track to 1 at its end.
    [[nodiscard]] double fraction() const noexcept { return fraction_; }

    /// Whether the edge is being dragged, in which case where it sits is a place being chosen
    /// rather than a place being played.
    [[nodiscard]] bool dragging() const noexcept { return dragging_; }

signals:
    /// Asks for playback to move to `fraction` of the track.
    void seek_requested(double fraction);

    /// The edge has moved under the mouse and has not been let go of yet.
    void dragged();

protected:
    void paintEvent(QPaintEvent* event) override;

    void mousePressEvent(QMouseEvent* event) override;

    void mouseMoveEvent(QMouseEvent* event) override;

    void mouseReleaseEvent(QMouseEvent* event) override;

private:
    /// Where `x` falls along the bar, from 0 at its left edge to 1 at its right.
    [[nodiscard]] double fraction_at(int x) const noexcept;

    double fraction_{0.0};
    bool dragging_{false};
};

} // namespace wiola::gui
