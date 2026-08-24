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

#include <cmath>

namespace wiola::gui {

namespace {

QString as_clock(units::Time time)
{
    const auto total = static_cast<int>(time.get<units::Sec>());

    return QString{"%1:%2"}.arg(total / 60).arg(total % 60, 2, 10, QChar{'0'});
}

} // namespace

MainWindow::MainWindow()
{
    setWindowTitle("Wiola Player");

    // Every widget is given this window as its parent, so Qt owns them and destroys them with it.
    open_button_ = new QPushButton{"Open", this};
    play_button_ = new QPushButton{"Play", this};
    stop_button_ = new QPushButton{"Stop", this};
    position_bar_ = new SeekBar{this};
    time_label_ = new QLabel{this};
    status_label_ = new QLabel{this};
    volume_slider_ = new QSlider{Qt::Horizontal, this};
    refresh_timer_ = new QTimer{this};

    volume_slider_->setRange(0, tuning::full_volume);
    volume_slider_->setValue(tuning::full_volume);
    volume_slider_->setFixedWidth(tuning::volume_slider_width);
    volume_slider_->setToolTip("Volume");

    auto* controls = new QHBoxLayout;
    controls->addWidget(open_button_);
    controls->addWidget(play_button_);
    controls->addWidget(stop_button_);
    controls->addWidget(time_label_);
    controls->addWidget(volume_slider_);

    auto* layout = new QVBoxLayout{this};
    layout->addWidget(position_bar_);
    layout->addLayout(controls);

    // Kept in the layout while it says nothing, so a message appearing does not resize the window.
    layout->addWidget(status_label_);

    connect(open_button_, &QPushButton::clicked, this, &MainWindow::choose_track);
    connect(play_button_, &QPushButton::clicked, this, &MainWindow::toggle_playback);
    connect(stop_button_, &QPushButton::clicked, this, &MainWindow::stop_playback);

    connect(volume_slider_, &QSlider::valueChanged, this, &MainWindow::set_volume);

    connect(position_bar_, &SeekBar::seek_requested, this, &MainWindow::seek_to);

    // The clock follows the drag rather than waiting for the next poll.
    connect(position_bar_, &SeekBar::dragged, this, &MainWindow::refresh);

    connect(refresh_timer_, &QTimer::timeout, this, &MainWindow::refresh);
    refresh_timer_->start(tuning::engine_poll_interval);

    refresh();
}

MainWindow::~MainWindow() = default;

bool MainWindow::load(const std::filesystem::path& path)
{
    std::unique_ptr<codec::Decoder> source{codec::open_file(path)};

    // The old player is stopped before the new one is built: a running device reads the chain
    // that building one reconfigures.
    player_.reset();
    player_ = source ? std::make_unique<engine::Player>(std::move(source), chain_) : nullptr;

    show_status(loaded() ? QString{} : QString{"cannot read that file"});
    setWindowTitle(loaded() ? QString::fromStdString(path.filename().string())
                            : QString{"Wiola Player"});
    position_bar_->set_fraction(0.0);
    refresh();

    return loaded();
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

    // The state says which of the three this press means: silence it, carry on, or begin - the
    // last of which also covers playing a track that has already finished.
    switch (player_->state()) {
    case engine::PlayerState::playing:
        static_cast<void>(player_->pause());
        return;

    case engine::PlayerState::paused:
        show_status(player_->resume() ? QString{} : QString{"no playback device"});
        return;

    default:
        show_status(player_->start() ? QString{} : QString{"no playback device"});
        return;
    }
}

void MainWindow::set_volume(int percent)
{
    const double position{static_cast<double>(percent) / tuning::full_volume};

    chain_.set_volume(static_cast<float>(std::pow(position, tuning::volume_curve)));
}

void MainWindow::show_status(const QString& message)
{
    status_label_->setText(message);
}

void MainWindow::stop_playback()
{
    if (!loaded())
        return;

    player_->stop();

    // Stopping is not pausing, which the play button already does: the next play begins at the
    // start of the track, so that is where the slider and the clock have to say playback is.
    player_->seek(units::Time{});
}

void MainWindow::seek_to(double fraction)
{
    if (loaded())
        player_->seek(player_->total_time() * fraction);
}

void MainWindow::refresh()
{
    play_button_->setEnabled(loaded());
    stop_button_->setEnabled(loaded());
    position_bar_->setEnabled(loaded());

    if (!loaded()) {
        play_button_->setText("Play");
        time_label_->setText("no track");

        return;
    }

    const units::Time total{player_->total_time()};

    // While the edge is being dragged it is a place being chosen, and that is the time worth
    // showing: a listener aiming for somewhere needs to see where they are aiming.
    const units::Time shown{
        position_bar_->dragging() ? total * position_bar_->fraction() : player_->time_played()};

    play_button_->setText(player_->playing() ? "Pause" : "Play");
    time_label_->setText(as_clock(shown) + " / " + as_clock(total));

    if (total == units::Time{})
        return;

    // A drag in progress is left alone by the bar itself.
    position_bar_->set_fraction(shown.get<units::Sec>() / total.get<units::Sec>());
}

} // namespace wiola::gui
