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

#include <audio/chain.hpp>
#include <audio/equalizer.hpp>
#include <audio/output.hpp>
#include <audio/source.hpp>
#include <audio/volume.hpp>
#include <codec/open.hpp>
#include <core/macros.hpp>
#include <engine/loader.hpp>
#include <engine/player.hpp>
#include <utils/units.hpp>

#include <filesystem>
#include <functional>
#include <memory>

namespace wiola::engine {

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

    /// Begins loading `path`. Reading a file is not quick enough to wait for, so this returns
    /// `loading` and whatever is playing keeps playing; `poll` is what finishes the job. A file
    /// that will not open leaves the track that was playing alone, and says why through
    /// `last_result`.
    codec::OpenResult load(const std::filesystem::path& path);

    /// Takes up a finished load, replacing whatever was playing. For the thread that called
    /// `load`, as often as it likes: a caller that draws should call it as it draws.
    void poll();

    /// Whether a file is still being read.
    [[nodiscard]] bool loading() const noexcept;

    /// How the last load went, which is `loading` until one has finished.
    [[nodiscard]] codec::OpenResult last_result() const noexcept;

    [[nodiscard]] bool loaded() const noexcept;

    /// Starts, pauses or resumes, whichever playback is due. False when the output could not
    /// be opened, and when there is nothing loaded.
    bool toggle();

    /// Ends playback and returns to the beginning of the track.
    void stop() noexcept;

    /// Moves playback to `position`, measured from the start of the track.
    void seek(units::Time position) noexcept;

    [[nodiscard]] Playback::State state() const noexcept;
    [[nodiscard]] bool playing() const noexcept;
    [[nodiscard]] units::Time time_played() const noexcept;
    [[nodiscard]] units::Time total_time() const noexcept;

    [[nodiscard]] audio::Volume& volume() noexcept { return chain_.volume(); }

    [[nodiscard]] audio::Equalizer& equalizer() noexcept { return chain_.equalizer(); }

private:
    struct Pipeline;

    audio::Chain chain_{audio::StreamSpec{}};
    Loader loader_;
    codec::OpenResult last_result_{codec::OpenResult::opened};
    OutputFactory make_output_;
    std::unique_ptr<Pipeline> pipeline_;
};

} // namespace wiola::engine
