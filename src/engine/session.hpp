/**
 * @file
 * @brief One track, everything it plays through, and what the listener asked of it.
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

#include <audio/device/output.hpp>
#include <audio/dsp/chain.hpp>
#include <audio/dsp/equalizer.hpp>
#include <audio/dsp/source.hpp>
#include <audio/dsp/volume.hpp>
#include <codec/decode/decoder.hpp>
#include <codec/decode/open.hpp>
#include <core/macros.hpp>
#include <engine/queue/playlist.hpp>
#include <engine/transport/playback.hpp>
#include <utils/units.hpp>

#include <filesystem>
#include <functional>
#include <memory>
#include <vector>

namespace wiola::engine {

class Loader;
class TagReader;

/**
 * Builds what a track is played through, and keeps it built while it plays.
 *
 * A buffer and an output are cut for the format of the track that is loaded, so opening another
 * one replaces them. The chain outlives them all: a setting belongs to the listener rather than
 * to what is playing.
 */
/// Builds the output a track is played through, from what that track plays.
using OutputFactory = std::function<std::unique_ptr<audio::Output>(audio::Source&)>;

class Session {
public:
    /// Plays through the system output.
    Session();

    /// Plays through whatever `make_output` builds, which is asked once per track.
    explicit Session(OutputFactory make_output);

    NO_COPY_SEMANTIC(Session);
    NO_MOVE_SEMANTIC(Session);

    ~Session();

    /// Plays `tracks` in the order given, standing at the first of them and reading it. What
    /// was in the list before is replaced. Reading a file is not quick enough to wait for, so
    /// this returns `loading` and whatever is playing keeps playing; `catch_up` is what finishes
    /// the job. A file that will not open leaves the track that was playing alone, and says why
    /// through `open_result`.
    codec::OpenResult open(std::vector<std::filesystem::path> tracks);

    /// Stands at the next track of the list and reads it, keeping whether something was playing.
    /// False when there is nowhere to go.
    bool next_track();

    /// The same, backwards.
    bool previous_track();

    /// Puts `tracks` at the end of the queue, leaving what is playing alone. With nothing
    /// loaded, the first of them is read so that there is something to press play on.
    void add(std::vector<std::filesystem::path> tracks);

    /// Takes the track that was put at `index` out of the queue. What is playing keeps playing,
    /// even when it is the one taken out: the queue says what comes next, not what is on.
    bool remove(std::size_t index);

    /// Empties the queue and ends playback. What was playing stays loaded, so it can be played
    /// again without opening it afresh.
    void clear();

    /// What is queued, and where in it playback stands. For showing a listener their list.
    [[nodiscard]] const Playlist& playlist() const noexcept;

    /// Plays the track that was put at `index`, keeping whether something was playing. False
    /// when there is no such track.
    bool play_track(std::size_t index);

    void set_repeat(Playlist::Repeat repeat) noexcept;
    [[nodiscard]] Playlist::Repeat repeat() const noexcept;

    /// Plays the list in an order drawn at random, or in the order it was given.
    void shuffle(bool on);
    [[nodiscard]] bool shuffled() const noexcept;

    /// Does whatever has fallen due since the last time: puts on a track that has finished
    /// being read, and takes up the next one when the last has run out. For the thread that
    /// opens tracks, as often as it likes - a caller that draws should call it as it draws.
    void catch_up();

    /// Whether a file is still being read.
    [[nodiscard]] bool reading() const noexcept;

    /// How the last open went, which is `loading` until the file has been read.
    [[nodiscard]] codec::OpenResult open_result() const noexcept;

    [[nodiscard]] bool loaded() const noexcept;

    /// The file that is loaded, or nothing when none is. A load that fails leaves it as it was,
    /// along with the track it names.
    [[nodiscard]] const std::filesystem::path& track() const noexcept;

    /// Starts, pauses or resumes, whichever playback is due. False when the output could not
    /// be opened, and when there is nothing loaded.
    bool play_or_pause();

    /// Ends playback and returns to the beginning of the track.
    void stop() noexcept;

    /// Moves playback to `position`, measured from the start of the track.
    void seek(units::Time position) noexcept;

    [[nodiscard]] Playback::State state() const noexcept;
    [[nodiscard]] bool playing() const noexcept;
    [[nodiscard]] units::Time time_played() const noexcept;
    [[nodiscard]] units::Time total_time() const noexcept;

    [[nodiscard]] audio::Volume& volume() noexcept { return volume_; }

    [[nodiscard]] audio::Equalizer& equalizer() noexcept { return equalizer_; }

private:
    struct Pipeline;

    /// What a track is played through, for as long as one format lasts: the device, what it
    /// pulls, and the relay that each track in turn is put behind.
    struct Out;

    /// What a finished read is for.
    enum class Waiting {
        /// Put it on, and leave it as a track ready to be played.
        install,

        /// Put it on and play it: what a track ending, or a listener skipping, asks for.
        play,
    };

    /// Starts reading `path`, and says what is to be done with it once it has been read.
    codec::OpenResult begin_reading(const std::filesystem::path& path, Waiting waiting);

    /// Puts on whatever has finished being read, and plays it if that is what it was read for.
    void take_up_finished_read();

    /// Starts reading what `paths` say about themselves - their tags - which is taken up as it
    /// comes back.
    void begin_reading_tags(std::vector<std::filesystem::path> paths);

    /// Puts what has been read about queued tracks into the queue.
    void take_up_finished_tags();

    /// Reads whatever the list now stands at, playing it if something was playing.
    codec::OpenResult read_current_track(bool playing);

    /// Whether a track has been read and is waiting to go on.
    [[nodiscard]] bool read_waiting() const noexcept;

    /// Puts the track that was read in place of whatever is loaded, from its beginning and not
    /// playing. False when nothing is waiting.
    bool install_read_track();

    /// Takes up the next track when one has run out. A listener who pressed stop, or a device
    /// that went away, is not a cue to play anything.
    void advance_if_ended();

    /// What a track is shaped by, and in what order: the bands first, then how loud, so that
    /// what a listener asks for last is the last thing done to the sound.
    audio::Chain chain_;
    audio::Equalizer& equalizer_{chain_.add<audio::Equalizer>(audio::StreamSpec{})};
    audio::Volume& volume_{chain_.add<audio::Volume>()};
    std::unique_ptr<Loader> loader_;
    std::unique_ptr<TagReader> tags_;
    Playlist playlist_;

    /// What is being read, and what was read: the second becomes the first once a file has
    /// opened and taken the place of whatever was playing.
    std::filesystem::path opening_;
    std::filesystem::path track_;

    Waiting waiting_{Waiting::install};

    /// A track that has been read and not yet put on, with the name it goes by.
    std::unique_ptr<codec::Decoder> ready_;
    std::filesystem::path ready_track_;
    codec::OpenResult last_result_{codec::OpenResult::opened};
    OutputFactory make_output_;
    /// Declared before the pipeline, so that a player is let go while what it drives is still
    /// there to be given back.
    std::unique_ptr<Out> out_;
    std::unique_ptr<Pipeline> pipeline_;
};

} // namespace wiola::engine
