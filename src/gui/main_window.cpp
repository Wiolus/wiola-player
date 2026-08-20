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

#include <codec/open.hpp>

#include <QFileDialog>
#include <QHBoxLayout>
#include <QVBoxLayout>

namespace wiola::gui {

namespace {

/// How often the window asks the engine where playback is.
constexpr int refresh_ms{100};

/// Positions along the slider. Seconds would be too coarse for a short track.
constexpr int slider_steps{1000};

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
    position_slider_ = new QSlider{Qt::Horizontal, this};
    time_label_ = new QLabel{this};
    refresh_timer_ = new QTimer{this};

    position_slider_->setRange(0, slider_steps);

    auto* controls = new QHBoxLayout;
    controls->addWidget(open_button_);
    controls->addWidget(play_button_);
    controls->addWidget(stop_button_);
    controls->addWidget(time_label_);

    auto* layout = new QVBoxLayout{this};
    layout->addWidget(position_slider_);
    layout->addLayout(controls);

    connect(open_button_, &QPushButton::clicked, this, &MainWindow::choose_track);
    connect(play_button_, &QPushButton::clicked, this, &MainWindow::toggle_playback);
    connect(stop_button_, &QPushButton::clicked, this, &MainWindow::stop_playback);

    // Seeking on every pixel of a drag would stop and start the device continuously, so the
    // position is only taken once the slider is let go.
    connect(position_slider_, &QSlider::sliderPressed, this, [this] { scrubbing_ = true; });
    connect(position_slider_, &QSlider::sliderReleased, this, &MainWindow::seek_to_slider);

    connect(refresh_timer_, &QTimer::timeout, this, &MainWindow::refresh);
    refresh_timer_->start(refresh_ms);

    refresh();
}

MainWindow::~MainWindow() = default;

bool MainWindow::load(const std::filesystem::path& path)
{
    // The player borrows the decoder, so it has to go before the decoder is replaced.
    player_.reset();
    source_ = codec::open_file(path);

    if (source_)
        player_ = std::make_unique<engine::Player>(*source_);

    setWindowTitle(source_ ? QString::fromStdString(path.filename().string())
                           : QString{"Wiola Player"});
    position_slider_->setValue(0);
    refresh();

    return source_ != nullptr;
}

void MainWindow::choose_track()
{
    const QString chosen{QFileDialog::getOpenFileName(this, "Open track", QString{},
        "Audio (*.wav *.wave *.flac *.mp3 *.mp2 *.mpga);;All files (*)")};

    if (chosen.isEmpty())
        return;

    if (!load(std::filesystem::path{chosen.toStdString()}))
        time_label_->setText("cannot read that file");
}

units::Time MainWindow::length() const
{
    if (!source_)
        return units::Time{};

    return units::Time{
        static_cast<double>(source_->num_frames()) / source_->spec().sample_rate.get<units::Hz>()};
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
        if (!player_->resume())
            time_label_->setText("no playback device");
        return;

    default:
        if (!player_->start())
            time_label_->setText("no playback device");
        return;
    }
}

void MainWindow::stop_playback()
{
    if (loaded())
        player_->stop();
}

void MainWindow::seek_to_slider()
{
    scrubbing_ = false;

    if (!loaded())
        return;

    const double fraction{static_cast<double>(position_slider_->value()) / slider_steps};

    player_->seek(length() * fraction);
}

void MainWindow::refresh()
{
    play_button_->setEnabled(loaded());
    stop_button_->setEnabled(loaded());
    position_slider_->setEnabled(loaded());

    if (!loaded()) {
        play_button_->setText("Play");
        time_label_->setText("no track");

        return;
    }

    const units::Time position{player_->position()};
    const units::Time total{length()};

    play_button_->setText(player_->playing() ? "Pause" : "Play");
    time_label_->setText(as_clock(position) + " / " + as_clock(total));

    if (scrubbing_ || total == units::Time{})
        return;

    const double fraction{position.get<units::Sec>() / total.get<units::Sec>()};

    position_slider_->setValue(static_cast<int>(fraction * slider_steps));
}

} // namespace wiola::gui
