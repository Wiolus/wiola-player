/**
 * @file
 * @brief The window a listener shapes the sound from.
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

#include <audio/equalizer.hpp>

#include <QCheckBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QSlider>
#include <QWidget>

namespace wiola::gui {

/**
 * One slider per band, and the two controls that apply to all of them.
 *
 * The bands are built from the equalizer each time the panel is shown, since a track can change
 * how many of them the format carries. Positions are read back from it rather than remembered
 * here, so what is shown is what is being applied.
 */
class EqualizerPanel final : public QWidget {
    Q_OBJECT

public:
    EqualizerPanel(audio::Equalizer& equalizer, QWidget* parent);

protected:
    void showEvent(QShowEvent* event) override;

private:
    /// Replaces the row of band sliders with one the current format asks for.
    void rebuild_bands();

    audio::Equalizer* equalizer_;

    QCheckBox* enabled_box_{nullptr};
    QSlider* preamp_slider_{nullptr};
    QLabel* preamp_value_{nullptr};
    QWidget* bands_{nullptr};
    QHBoxLayout* bands_row_{nullptr};
};

} // namespace wiola::gui
