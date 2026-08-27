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

#include "equalizer_panel.hpp"

#include "tuning.hpp"

#include <audio/tuning.hpp>
#include <utils/units.hpp>

#include <QVBoxLayout>

#include <cmath>
#include <cstddef>

namespace wiola::gui {

namespace {

/// A center as a band is labelled: whole hertz up to a thousand, then thousands.
QString as_label(units::Frequency center)
{
    const double hz{center.get<units::Hz>()};

    if (hz < 1000.0)
        return QString::number(qRound(hz));

    return QString{"%1k"}.arg(qRound(hz / 1000.0));
}

/// A slider position as the gain it stands for.
constexpr double as_db(int steps)
{
    return static_cast<double>(steps) / tuning::gain_steps_per_db;
}

/// A gain as it is read off the panel: signed, and to a tenth of a decibel.
QString as_gain(int steps)
{
    const double db{as_db(steps)};

    return (steps > 0 ? QString{"+"} : QString{}) + QString::number(db, 'f', 1);
}

/// The slider position a gain in decibels stands on.
int as_steps(float db)
{
    return static_cast<int>(std::lround(db * tuning::gain_steps_per_db));
}

constexpr int band_limit{
    static_cast<int>(audio::tuning::max_band_gain_db) * tuning::gain_steps_per_db};

} // namespace

EqualizerPanel::EqualizerPanel(EqualizerControl& equalizer, QWidget* parent)
    : QWidget{parent, Qt::Window}
    , equalizer_{equalizer}
{
    setWindowTitle("Equalizer");

    enabled_box_ = new QCheckBox{"Equalizer", this};
    enabled_box_->setChecked(equalizer_.enabled());

    bands_row_ = new QHBoxLayout;

    auto* layout = new QVBoxLayout{this};
    layout->addWidget(enabled_box_);
    layout->addLayout(bands_row_);

    connect(enabled_box_, &QCheckBox::toggled, this,
        [this](bool on) { equalizer_.set_enabled(on); });

    rebuild_bands();
}

void EqualizerPanel::showEvent(QShowEvent* event)
{
    QWidget::showEvent(event);

    rebuild_bands();
}

void EqualizerPanel::rebuild_bands()
{
    delete bands_;

    bands_ = new QWidget{this};
    bands_row_->addWidget(bands_);

    auto* row = new QHBoxLayout{bands_};

    for (std::size_t i = 0; i < equalizer_.num_bands(); ++i) {
        auto* slider = new QSlider{Qt::Vertical, bands_};

        slider->setRange(-band_limit, band_limit);
        slider->setSingleStep(tuning::gain_step);
        slider->setValue(as_steps(equalizer_.band_gain(i)));
        slider->setFixedHeight(tuning::band_slider_height);

        auto* gain = new QLabel{as_gain(slider->value()), bands_};
        gain->setFixedWidth(tuning::band_column_width);
        gain->setAlignment(Qt::AlignHCenter);

        auto* center = new QLabel{as_label(equalizer_.band_center(i)), bands_};
        center->setFixedWidth(tuning::band_column_width);
        center->setAlignment(Qt::AlignHCenter);

        connect(slider, &QSlider::valueChanged, this, [this, i, gain](int steps) {
            equalizer_.set_band_gain(i, static_cast<float>(as_db(steps)));
            gain->setText(as_gain(steps));
        });

        auto* band = new QVBoxLayout;
        band->addWidget(gain);
        band->addWidget(slider, 0, Qt::AlignHCenter);
        band->addWidget(center);

        row->addLayout(band);
    }
}

} // namespace wiola::gui
