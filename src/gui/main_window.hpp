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

#pragma once

#include "equalizer_panel.hpp"
#include "playlist_control.hpp"
#include "playlist_view.hpp"
#include "seek_bar.hpp"
#include "volume_control.hpp"

#include <codec/open.hpp>
#include <engine/session.hpp>
#include <utils/units.hpp>

#include <QIcon>
#include <QLabel>
#include <QPushButton>
#include <QSettings>
#include <QSlider>
#include <QStyle>
#include <QTimer>
#include <QWidget>

#include <filesystem>
#include <memory>
#include <optional>
#include <vector>

namespace wiola::gui {

/**
 * Shows one track and the controls for it.
 *
 * The engine publishes nothing, so the window asks it where playback is on a timer. That is what
 * an event loop is for, and it keeps the engine free of any knowledge that a window exists.
 *
 * A window opens with nothing loaded and is given a track later, so that choosing one is an
 * ordinary thing to do rather than something only the command line can arrange.
 */
class MainWindow final : public QWidget {
    Q_OBJECT

public:
    MainWindow();

    ~MainWindow() override;

    /// Begins playing `tracks`, in the order given. Reading a file takes as long as it takes, so
    /// what is playing keeps playing and the window says what happened once it has been read.
    void open(std::vector<std::filesystem::path> tracks);

private:
    /// Asks for files and plays them, in the order they were chosen.
    void choose_tracks();

    /// Asks a listener which files they mean, and answers with nothing when they say none.
    [[nodiscard]] std::vector<std::filesystem::path> ask_for_tracks();

    /// Asks for files and puts them at the end of the queue.
    void add_tracks();

    /// Takes whatever rows are picked out of the queue.
    void remove_chosen();

    /// Empties the queue.
    void clear_queue();

    /// Plays the track the listener picked out of the queue.
    void play_chosen(std::size_t index);

    /// Plays the track before or after this one in the list.
    void play_previous();
    void play_next();

    /// Steps between playing the list once, playing it round and round, and playing one track
    /// over: the three a listener cycles through with one button.
    void cycle_repeat();

    /// Plays the list in the order it was given, or in one drawn at random.
    void set_shuffled(bool shuffled);

    /// Puts `icon` on `button`, with `says` for whoever cannot read a picture.
    void show_button(QPushButton& button, QStyle::StandardPixmap icon, const QString& says);

    /// Shows the play button as pausing or as playing. Does nothing when it already shows that
    /// one: a window refreshing ten times a second would otherwise ask the style for an icon it
    /// is already wearing.
    void show_play_button(bool pausing);

    /// Says which of the three repeats is on, and whether the order is shuffled. Whatever changes
    /// either must call this: refreshing no longer asks.
    void show_list_buttons();

    /// Starts playing, or resumes after a pause. The first press is what opens the device.
    void toggle_playback();

    void stop_playback();

    /// Opens the equalizer, building it the first time it is asked for.
    void show_equalizer();

    /// Sets how loud the output is, from a slider position out of a hundred, and keeps it for the
    /// runs after this one.
    void set_volume(int percent);

    /// Lets the slider ask for more than arrived, or stops it doing so.
    void set_boosted(bool boosted);

    /// Says how loud the output is, as the slider has it.
    void show_volume();

    /// Moves playback to `fraction` of the loaded track.
    void seek_to(double fraction);

    /// Takes up a load that has finished, and says how it went.
    void take_up_load();

    /// Reads the engine and refreshes what is shown.
    void refresh();

    /// Says `message`, or clears what was said when it is empty. Refreshing leaves this alone,
    /// so a message stays readable rather than lasting until the next poll.
    void show_status(const QString& message);

    [[nodiscard]] bool loaded() const noexcept { return session_.loaded(); }

    engine::Session session_;

    /// What the window has already said about a load, so that it says it once.
    codec::OpenResult said_{codec::OpenResult::opened};

    /// What it has already said about playback going wrong, for the same reason.
    bool said_fault_{false};

    /// The track the title names, and where in the queue it stood: a listener may queue the
    /// same file twice, and moving from one of them to the other is still a new track.
    std::filesystem::path named_;
    std::optional<std::size_t> named_place_;

    /// The two faces of the play button, taken from the style once: asking it for one is a
    /// lookup, and the button changes far less often than the window is drawn.
    QIcon play_icon_;
    QIcon pause_icon_;

    /// Which of the two it is wearing, or nothing before it wears either.
    std::optional<bool> showing_pause_;

    /// The file a run leaves things in.
    QSettings settings_;
    PlaylistControl playlist_;
    VolumeControl volume_;
    EqualizerControl equalizer_;

    QPushButton* open_button_{nullptr};
    QPushButton* previous_button_{nullptr};
    QPushButton* play_button_{nullptr};
    QPushButton* next_button_{nullptr};
    QPushButton* stop_button_{nullptr};
    QPushButton* repeat_button_{nullptr};
    QPushButton* shuffle_button_{nullptr};
    QPushButton* add_button_{nullptr};
    QPushButton* remove_button_{nullptr};
    QPushButton* clear_button_{nullptr};
    SeekBar* position_bar_{nullptr};
    PlaylistView* playlist_view_{nullptr};
    QSlider* volume_slider_{nullptr};
    QLabel* volume_value_{nullptr};
    QPushButton* equalizer_button_{nullptr};
    QPushButton* boost_button_{nullptr};
    EqualizerPanel* equalizer_panel_{nullptr};
    QLabel* track_label_{nullptr};
    QLabel* elapsed_label_{nullptr};
    QLabel* total_label_{nullptr};
    QLabel* status_label_{nullptr};
    QTimer* refresh_timer_{nullptr};
};

} // namespace wiola::gui
