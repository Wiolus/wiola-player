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

#include "clock.hpp"
#include "tuning.hpp"

#include <codec/decoder.hpp>
#include <codec/open.hpp>

#include <QFileDialog>
#include <QFont>
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

} // namespace

MainWindow::MainWindow()
    // A file of its own on both platforms, rather than the registry on one of them.
    : settings_{QSettings::IniFormat, QSettings::UserScope, "wiola-player", "settings"}
    , playlist_{session_, settings_}
    , volume_{session_.volume(), settings_}
    , equalizer_{session_.equalizer(), settings_}
{
    setWindowTitle("Wiola Player");
    setMinimumWidth(tuning::window_width);

    // Every widget is given this window as its parent, so Qt owns them and destroys them with it.
    open_button_ = new QPushButton{this};
    previous_button_ = new QPushButton{this};
    play_button_ = new QPushButton{this};
    next_button_ = new QPushButton{this};
    stop_button_ = new QPushButton{this};

    // Pictures for what a listener does, and words for what is set: the style draws these, so
    // there is nothing to ship beside the program and they look native wherever it runs.
    show_button(*open_button_, QStyle::SP_DirOpenIcon, "Open tracks");
    show_button(*previous_button_, QStyle::SP_MediaSkipBackward, "Previous track");
    show_button(*play_button_, QStyle::SP_MediaPlay, "Play");
    show_button(*next_button_, QStyle::SP_MediaSkipForward, "Next track");
    show_button(*stop_button_, QStyle::SP_MediaStop, "Stop");

    play_button_->setFixedWidth(tuning::play_button_width);
    repeat_button_ = new QPushButton{"Repeat: off", this};
    shuffle_button_ = new QPushButton{"Shuffle", this};
    add_button_ = new QPushButton{"Add", this};
    remove_button_ = new QPushButton{"Remove", this};
    clear_button_ = new QPushButton{"Clear", this};

    shuffle_button_->setCheckable(true);
    equalizer_button_ = new QPushButton{"EQ", this};
    boost_button_ = new QPushButton{"+", this};
    volume_value_ = new QLabel{this};
    position_bar_ = new SeekBar{this};
    playlist_view_ = new PlaylistView{this};
    track_label_ = new QLabel{this};
    elapsed_label_ = new QLabel{this};
    total_label_ = new QLabel{this};
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

    QFont title{track_label_->font()};
    title.setPointSize(title.pointSize() + tuning::track_name_point_size_step);
    title.setBold(true);
    track_label_->setFont(title);

    total_label_->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

    // What is playing: its name, where it has reached, and how far that is through it.
    auto* times = new QHBoxLayout;
    times->addWidget(elapsed_label_);
    times->addStretch();
    times->addWidget(total_label_);

    // How it plays. Everything here acts on the track that is on.
    auto* transport = new QHBoxLayout;
    transport->addWidget(previous_button_);
    transport->addWidget(play_button_);
    transport->addWidget(next_button_);
    transport->addWidget(stop_button_);
    transport->addStretch();
    transport->addWidget(volume_slider_);
    transport->addWidget(volume_value_);
    transport->addWidget(boost_button_);

    // What the list does. Repeat and shuffle change what plays next rather than how this one
    // plays, which is why they are not up there with the transport.
    auto* list = new QHBoxLayout;
    list->addWidget(open_button_);
    list->addWidget(repeat_button_);
    list->addWidget(shuffle_button_);
    list->addStretch();
    list->addWidget(equalizer_button_);

    auto* layout = new QVBoxLayout{this};
    layout->addWidget(track_label_);
    layout->addWidget(position_bar_);
    layout->addLayout(times);
    layout->addLayout(transport);
    layout->addLayout(list);
    layout->addWidget(playlist_view_);

    // What is done to the queue, under the queue it is done to.
    auto* queue = new QHBoxLayout;
    queue->addWidget(add_button_);
    queue->addWidget(remove_button_);
    queue->addWidget(clear_button_);
    queue->addStretch();

    layout->addLayout(queue);

    // Kept in the layout while it says nothing, so a message appearing does not resize the window.
    layout->addWidget(status_label_);

    connect(open_button_, &QPushButton::clicked, this, &MainWindow::choose_tracks);
    connect(previous_button_, &QPushButton::clicked, this, &MainWindow::play_previous);
    connect(next_button_, &QPushButton::clicked, this, &MainWindow::play_next);
    connect(playlist_view_, &PlaylistView::track_chosen, this, &MainWindow::play_chosen);
    connect(playlist_view_, &PlaylistView::removal_asked, this, &MainWindow::remove_chosen);
    connect(add_button_, &QPushButton::clicked, this, &MainWindow::add_tracks);
    connect(remove_button_, &QPushButton::clicked, this, &MainWindow::remove_chosen);
    connect(clear_button_, &QPushButton::clicked, this, &MainWindow::clear_queue);
    connect(repeat_button_, &QPushButton::clicked, this, &MainWindow::cycle_repeat);
    connect(shuffle_button_, &QPushButton::toggled, this, &MainWindow::set_shuffled);
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

    if (const std::size_t gone{playlist_.restore()}; gone != 0)
        show_status(QString{"%1 queued track(s) are no longer where they were"}.arg(gone));

    refresh();
}

MainWindow::~MainWindow() = default;

void MainWindow::open(std::vector<std::filesystem::path> tracks)
{
    if (tracks.empty())
        return;

    const QString first{QString::fromStdString(tracks.front().filename().string())};

    said_ = session_.open(std::move(tracks));

    playlist_.save();
    show_status(QString{"opening "} + first + "...");
    refresh();
}

std::vector<std::filesystem::path> MainWindow::ask_for_tracks()
{
    const QStringList chosen{QFileDialog::getOpenFileNames(this, "Choose tracks", QString{},
        "Audio (*.wav *.wave *.flac *.mp3 *.mp2 *.mpga);;All files (*)")};

    std::vector<std::filesystem::path> tracks;
    tracks.reserve(static_cast<std::size_t>(chosen.size()));

    for (const QString& name : chosen)
        tracks.emplace_back(name.toStdString());

    return tracks;
}

void MainWindow::choose_tracks()
{
    // Whether the files can be read is said by playing them.
    open(ask_for_tracks());
}

void MainWindow::add_tracks()
{
    std::vector<std::filesystem::path> tracks{ask_for_tracks()};

    if (tracks.empty())
        return;

    session_.add(std::move(tracks));
    playlist_.save();
    refresh();
}

void MainWindow::remove_chosen()
{
    for (const std::size_t row : playlist_view_->chosen_rows())
        static_cast<void>(session_.remove(row));

    playlist_.save();
    refresh();
}

void MainWindow::clear_queue()
{
    session_.clear();
    playlist_.save();
    refresh();
}

void MainWindow::play_chosen(std::size_t index)
{
    static_cast<void>(session_.play_track(index));
    refresh();
}

void MainWindow::play_previous()
{
    static_cast<void>(session_.previous_track());
    refresh();
}

void MainWindow::play_next()
{
    static_cast<void>(session_.next_track());
    refresh();
}

void MainWindow::cycle_repeat()
{
    using Repeat = engine::Playlist::Repeat;

    switch (session_.repeat()) {
    case Repeat::none:
        session_.set_repeat(Repeat::all);
        break;

    case Repeat::all:
        session_.set_repeat(Repeat::track);
        break;

    case Repeat::track:
        session_.set_repeat(Repeat::none);
        break;
    }

    show_list_buttons();
}

void MainWindow::set_shuffled(bool shuffled)
{
    session_.shuffle(shuffled);
    show_list_buttons();
}

void MainWindow::show_button(QPushButton& button, QStyle::StandardPixmap icon, const QString& says)
{
    button.setIcon(style()->standardIcon(icon));
    button.setToolTip(says);

    // The word is gone, so the tooltip is what a listener is told: it is also what a screen
    // reader is left with.
    button.setAccessibleName(says);

    if (&button != play_button_)
        button.setFixedWidth(tuning::icon_button_width);
}

void MainWindow::show_list_buttons()
{
    using Repeat = engine::Playlist::Repeat;

    switch (session_.repeat()) {
    case Repeat::none:
        repeat_button_->setText("Repeat: off");
        break;

    case Repeat::all:
        repeat_button_->setText("Repeat: all");
        break;

    case Repeat::track:
        repeat_button_->setText("Repeat: one");
        break;
    }

    shuffle_button_->setChecked(session_.shuffled());
}

void MainWindow::take_up_load()
{
    session_.catch_up();

    const codec::OpenResult result{session_.open_result()};

    // Said once: a window that repeated itself every turn would overwrite whatever else it has
    // to say.
    if (result != said_ && result != codec::OpenResult::loading) {
        said_ = result;

        const bool opened{result == codec::OpenResult::opened};

        show_status(opened ? QString{} : as_status(result));

        if (opened)
            position_bar_->set_fraction(0.0);
    }

    // The title follows what is playing, so a list moving on to the next track renames the
    // window without anyone asking it to.
    if (session_.track() != named_ || session_.playlist().position() != named_place_) {
        named_ = session_.track();
        named_place_ = session_.playlist().position();

        const QString name{named_.empty() ? QString{"no track"}
                                          : QString::fromStdString(named_.filename().string())};

        setWindowTitle(named_.empty() ? QString{"Wiola Player"} : name);
        track_label_->setText(name);
        position_bar_->set_fraction(0.0);
        said_fault_ = false;

        // Where the queue stands has moved, which is worth keeping even if nothing was added.
        playlist_.save();
    }

    // A device that goes away is not a listener pressing stop, and is the one thing about
    // playback worth interrupting them for.
    const bool faulted{session_.state() == engine::Playback::State::faulted};

    if (faulted && !said_fault_) {
        said_fault_ = true;

        show_status(QString{"lost the playback device"});
    }
}

void MainWindow::toggle_playback()
{
    if (!loaded())
        return;

    // Pausing cannot fail, so a refusal is always an output that would not open.
    const bool pausing{session_.state() == engine::Playback::State::playing};

    show_status(session_.play_or_pause() || pausing ? QString{} : QString{"no playback device"});
}

void MainWindow::stop_playback()
{
    if (!loaded())
        return;

    session_.stop();
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

void MainWindow::seek_to(double fraction)
{
    if (loaded())
        session_.seek(session_.total_time() * fraction);
}

void MainWindow::show_status(const QString& message)
{
    status_label_->setText(message);
}

void MainWindow::refresh()
{
    take_up_load();

    play_button_->setEnabled(loaded());
    stop_button_->setEnabled(loaded());
    position_bar_->setEnabled(loaded());
    previous_button_->setEnabled(loaded());
    next_button_->setEnabled(loaded());

    show_list_buttons();
    playlist_view_->show_playlist(session_.playlist());

    if (!loaded()) {
        show_button(*play_button_, QStyle::SP_MediaPlay, "Play");
        track_label_->setText("no track");
        elapsed_label_->clear();
        total_label_->clear();

        return;
    }

    const units::Time total{session_.total_time()};

    // While the edge is being dragged it is a place being chosen, and that is the time worth
    // showing: a listener aiming for somewhere needs to see where they are aiming.
    const units::Time shown{
        position_bar_->dragging() ? total * position_bar_->fraction() : session_.time_played()};

    if (session_.playing())
        show_button(*play_button_, QStyle::SP_MediaPause, "Pause");
    else
        show_button(*play_button_, QStyle::SP_MediaPlay, "Play");

    elapsed_label_->setText(as_clock(shown));
    total_label_->setText(as_clock(total));

    if (total == units::Time{})
        return;

    // A drag in progress is left alone by the bar itself.
    position_bar_->set_fraction(shown.get<units::Sec>() / total.get<units::Sec>());
}

} // namespace wiola::gui
