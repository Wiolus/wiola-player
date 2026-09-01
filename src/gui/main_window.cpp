/**
 * @file
 * @brief The window a listener drives one track from.
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

#include "main_window.hpp"

#include "tuning.hpp"

#include <codec/decoder.hpp>
#include <codec/open.hpp>

#include <QFileDialog>
#include <QHBoxLayout>
#include <QVBoxLayout>

#include <algorithm>
#include <cmath>

namespace wiola::gui {

namespace {

/// What to say about a file that would not open.
QString as_status(codec::OpenResult result)
{
    switch (result) {
    case codec::OpenResult::unsupported:
        return QString{"unsupported format"};

    case codec::OpenResult::damaged:
        return QString{"that file is damaged"};

    case codec::OpenResult::unreadable:
        return QString{"cannot read that file"};

    default:
        return QString{"cannot read that file"};
    }
}

QString as_clock(units::Time time)
{
    const auto total = static_cast<int>(time.get<units::Sec>());

    return QString{"%1:%2"}.arg(total / 60).arg(total % 60, 2, 10, QChar{'0'});
}

} // namespace

MainWindow::MainWindow()
    // A file of its own on both platforms, rather than the registry on one of them.
    : settings_{QSettings::IniFormat, QSettings::UserScope, "wiola-player", "settings"}
    , volume_{session_.volume(), settings_}
    , equalizer_{session_.equalizer(), settings_}
{
    setWindowTitle("Wiola Player");

    // Every widget is given this window as its parent, so Qt owns them and destroys them with it.
    open_button_ = new QPushButton{"Open", this};
    play_button_ = new QPushButton{"Play", this};
    stop_button_ = new QPushButton{"Stop", this};
    equalizer_button_ = new QPushButton{"EQ", this};
    boost_button_ = new QPushButton{"+", this};
    volume_value_ = new QLabel{this};
    position_bar_ = new SeekBar{this};
    time_label_ = new QLabel{this};
    status_label_ = new QLabel{this};
    volume_slider_ = new QSlider{Qt::Horizontal, this};
    refresh_timer_ = new QTimer{this};

    const int position{volume_.restore()};

    equalizer_.restore();

    boost_button_->setCheckable(true);
    boost_button_->setChecked(volume_.boosted());
    boost_button_->setFixedWidth(tuning::boost_button_width);
    boost_button_->setToolTip("Past full volume");

    volume_value_->setFixedWidth(tuning::volume_value_width);

    volume_slider_->setRange(0, volume_.max_position());
    volume_slider_->setValue(position);

    show_volume();
    volume_slider_->setFixedWidth(tuning::volume_slider_width);
    volume_slider_->setToolTip("Volume");

    auto* controls = new QHBoxLayout;
    controls->addWidget(open_button_);
    controls->addWidget(play_button_);
    controls->addWidget(stop_button_);
    controls->addWidget(equalizer_button_);
    controls->addWidget(time_label_);
    controls->addWidget(volume_slider_);
    controls->addWidget(volume_value_);
    controls->addWidget(boost_button_);

    auto* layout = new QVBoxLayout{this};
    layout->addWidget(position_bar_);
    layout->addLayout(controls);

    // Kept in the layout while it says nothing, so a message appearing does not resize the window.
    layout->addWidget(status_label_);

    connect(open_button_, &QPushButton::clicked, this, &MainWindow::choose_track);
    connect(play_button_, &QPushButton::clicked, this, &MainWindow::toggle_playback);
    connect(stop_button_, &QPushButton::clicked, this, &MainWindow::stop_playback);

    connect(equalizer_button_, &QPushButton::clicked, this, &MainWindow::show_equalizer);
    connect(volume_slider_, &QSlider::valueChanged, this, &MainWindow::set_volume);
    connect(boost_button_, &QPushButton::toggled, this, &MainWindow::set_boosted);

    connect(position_bar_, &SeekBar::seek_requested, this, &MainWindow::seek_to);

    // The clock follows the drag rather than waiting for the next poll.
    connect(position_bar_, &SeekBar::dragged, this, &MainWindow::refresh);

    connect(refresh_timer_, &QTimer::timeout, this, &MainWindow::refresh);
    refresh_timer_->start(tuning::engine_poll_interval);

    refresh();
}

MainWindow::~MainWindow() = default;

void MainWindow::load(const std::filesystem::path& path)
{
    said_ = session_.open(path);

    show_status(QString{"opening "} + QString::fromStdString(path.filename().string()) + "...");
    refresh();
}

void MainWindow::take_up_load()
{
    session_.catch_up();

    const codec::OpenResult result{session_.open_result()};

    // Said once: a window that repeated itself every turn would overwrite whatever else it has
    // to say.
    if (result == said_ || result == codec::OpenResult::loading)
        return;

    said_ = result;

    const bool opened{result == codec::OpenResult::opened};

    show_status(opened ? QString{} : as_status(result));
    setWindowTitle(opened ? QString::fromStdString(session_.track().filename().string())
                          : windowTitle());

    if (opened)
        position_bar_->set_fraction(0.0);
}

void MainWindow::choose_track()
{
    const QString chosen{QFileDialog::getOpenFileName(this, "Open track", QString{},
        "Audio (*.wav *.wave *.flac *.mp3 *.mp2 *.mpga);;All files (*)")};

    if (chosen.isEmpty())
        return;

    // Whether the file could be read is said by loading it.
    load(std::filesystem::path{chosen.toStdString()});
}

void MainWindow::toggle_playback()
{
    if (!loaded())
        return;

    // Pausing cannot fail, so a refusal is always an output that would not open.
    const bool pausing{session_.state() == engine::Playback::State::playing};

    show_status(session_.play_or_pause() || pausing ? QString{} : QString{"no playback device"});
}

void MainWindow::show_equalizer()
{
    if (equalizer_panel_ == nullptr)
        equalizer_panel_ = new EqualizerPanel{equalizer_, this};

    equalizer_panel_->show();
    equalizer_panel_->raise();
}

void MainWindow::set_volume(int percent)
{
    volume_.set_position(percent);

    show_volume();
}

void MainWindow::show_volume()
{
    volume_value_->setText(QString::number(volume_.position()) + "%");
}

void MainWindow::set_boosted(bool boosted)
{
    volume_.set_boosted(boosted);

    // The slider is asked to hold what the control now allows, and says what it settled on.
    volume_slider_->setRange(0, volume_.max_position());
    volume_slider_->setValue(volume_.position());

    show_volume();
}

void MainWindow::show_status(const QString& message)
{
    status_label_->setText(message);
}

void MainWindow::stop_playback()
{
    if (!loaded())
        return;

    session_.stop();
}

void MainWindow::seek_to(double fraction)
{
    if (loaded())
        session_.seek(session_.total_time() * fraction);
}

void MainWindow::refresh()
{
    take_up_load();

    play_button_->setEnabled(loaded());
    stop_button_->setEnabled(loaded());
    position_bar_->setEnabled(loaded());

    if (!loaded()) {
        play_button_->setText("Play");
        time_label_->setText("no track");

        return;
    }

    const units::Time total{session_.total_time()};

    // While the edge is being dragged it is a place being chosen, and that is the time worth
    // showing: a listener aiming for somewhere needs to see where they are aiming.
    const units::Time shown{
        position_bar_->dragging() ? total * position_bar_->fraction() : session_.time_played()};

    play_button_->setText(session_.playing() ? "Pause" : "Play");
    time_label_->setText(as_clock(shown) + " / " + as_clock(total));

    if (total == units::Time{})
        return;

    // A drag in progress is left alone by the bar itself.
    position_bar_->set_fraction(shown.get<units::Sec>() / total.get<units::Sec>());
}

} // namespace wiola::gui
