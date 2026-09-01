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
#include "seek_bar.hpp"
#include "volume_control.hpp"

#include <codec/open.hpp>
#include <engine/session.hpp>
#include <utils/units.hpp>

#include <QLabel>
#include <QPushButton>
#include <QSettings>
#include <QSlider>
#include <QTimer>
#include <QWidget>

#include <filesystem>
#include <memory>
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

    /// Plays the track before or after this one in the list.
    void play_previous();
    void play_next();

    /// Steps between playing the list once, playing it round and round, and playing one track
    /// over: the three a listener cycles through with one button.
    void cycle_repeat();

    /// Plays the list in the order it was given, or in one drawn at random.
    void set_shuffled(bool shuffled);

    /// Says which of the three repeats is on, and whether the order is shuffled.
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

    /// The track the title names, so that a list moving on renames it.
    std::filesystem::path named_;

    /// The file a run leaves things in.
    QSettings settings_;
    VolumeControl volume_;
    EqualizerControl equalizer_;

    QPushButton* open_button_{nullptr};
    QPushButton* previous_button_{nullptr};
    QPushButton* play_button_{nullptr};
    QPushButton* next_button_{nullptr};
    QPushButton* stop_button_{nullptr};
    QPushButton* repeat_button_{nullptr};
    QPushButton* shuffle_button_{nullptr};
    SeekBar* position_bar_{nullptr};
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
