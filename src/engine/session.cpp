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

#include <engine/session.hpp>

#include "loader.hpp"
#include "player.hpp"

#include "tuning.hpp"

#include <audio/buffer_source.hpp>
#include <audio/device.hpp>
#include <audio/relay.hpp>
#include <audio/shaped_source.hpp>
#include <audio/stream_spec.hpp>
#include <codec/decoder.hpp>
#include <codec/open.hpp>
#include <lockfree/spsc_ring_buffer.hpp>

#include <memory>
#include <optional>
#include <random>
#include <vector>

#include <utility>

namespace wiola::engine {

/// Declared in the order they are wired, so that each is built before whoever reads it and torn
/// down after.
struct Session::Pipeline {
    Pipeline(std::unique_ptr<codec::Decoder> source, audio::Output& output)
        : buffer{source->spec().samples_per(tuning::buffer_duration)}
        , decoded{source->spec(), buffer.consumer()}
        , player{std::move(source), buffer.producer(), output}
    {
    }

    lockfree::SPSCRingBuffer<float> buffer;
    audio::BufferSource decoded;
    Player player;
};

/// Built for a format and kept for as long as tracks come in it, so that a track change is a
/// device stopping and starting rather than one closing and another opening.
struct Session::Out {
    Out(audio::StreamSpec spec, audio::Chain& chain, const OutputFactory& make_output)
        : spec{spec}
        , relay{spec}
        , shaped{relay, chain}
        , output{make_output(shaped)}
    {
    }

    audio::StreamSpec spec;
    audio::Relay relay;
    audio::ShapedSource shaped;
    std::unique_ptr<audio::Output> output;
};

Session::Session()
    : Session{[](audio::Source& source) { return std::make_unique<audio::Device>(source); }}
{
}

Session::Session(OutputFactory make_output)
    : make_output_{std::move(make_output)}
    , loader_{std::make_unique<Loader>()}
{
}

Session::~Session() = default;

codec::OpenResult Session::open(const std::filesystem::path& path)
{
    // One file is a list of one: everything that plays, plays from the list.
    playlist_.set({path});

    return begin_reading(path, Waiting::install);
}

codec::OpenResult Session::open(std::vector<std::filesystem::path> tracks)
{
    playlist_.set(std::move(tracks));

    if (playlist_.empty())
        return last_result_;

    return begin_reading(playlist_.current(), Waiting::install);
}

bool Session::next_track()
{
    const bool was_playing{playing()};

    if (!playlist_.next())
        return false;

    static_cast<void>(read_current_track(was_playing));

    return true;
}

bool Session::previous_track()
{
    const bool was_playing{playing()};

    if (!playlist_.previous())
        return false;

    static_cast<void>(read_current_track(was_playing));

    return true;
}

void Session::set_repeat(Playlist::Repeat repeat) noexcept
{
    playlist_.set_repeat(repeat);
}

Playlist::Repeat Session::repeat() const noexcept
{
    return playlist_.repeat();
}

void Session::shuffle(bool on)
{
    if (!on) {
        playlist_.unshuffle();

        return;
    }

    std::random_device seeds;

    playlist_.shuffle(seeds());
}

bool Session::shuffled() const noexcept
{
    return playlist_.shuffled();
}

codec::OpenResult Session::begin_reading(const std::filesystem::path& path, Waiting waiting)
{
    // Whatever was read and not put on was read for a track nobody is waiting for any more.
    ready_.reset();
    ready_track_.clear();

    loader_->start(path);
    opening_ = path;
    waiting_ = waiting;
    last_result_ = codec::OpenResult::loading;

    return last_result_;
}

codec::OpenResult Session::read_current_track(bool playing)
{
    return begin_reading(playlist_.current(), playing ? Waiting::play : Waiting::install);
}

void Session::catch_up()
{
    take_up_finished_read();
    advance_if_ended();
}

void Session::take_up_finished_read()
{
    std::optional<codec::Opened> opened{loader_->take()};

    if (!opened.has_value())
        return;

    if (!*opened) {
        // Nothing is torn down: a file that would not open is no reason to silence the one that
        // did.
        last_result_ = opened->result;

        return;
    }

    ready_ = std::move(opened->decoder);
    ready_track_ = opening_;
    last_result_ = codec::OpenResult::opened;

    const bool play{waiting_ == Waiting::play};

    if (install_read_track() && play)
        static_cast<void>(play_or_pause());
}

void Session::advance_if_ended()
{
    // A track that ran out is the cue to play the next. A listener who pressed stop is not, and
    // neither is a device that went away.
    if (state() != Playback::State::ended)
        return;

    // One already on its way is the same cue, answered.
    if (reading() || read_waiting())
        return;

    if (!playlist_.next())
        return;

    static_cast<void>(read_current_track(true));
}

bool Session::read_waiting() const noexcept
{
    return ready_ != nullptr;
}

bool Session::install_read_track()
{
    if (!read_waiting())
        return false;

    std::unique_ptr<codec::Decoder> source{std::move(ready_)};
    const audio::StreamSpec spec{source->spec()};

    // Nothing to play, before there is nothing to play from: a device asking after this hears
    // silence rather than reading a track that has been let go. The player's end stops the device
    // anyway, but that is an ordering to keep, and this is not.
    if (out_)
        out_->relay.point_at_nothing();

    pipeline_.reset();

    chain_.configure(spec);

    // A device is opened for one format. Another of the same needs no more than the one that is
    // already open.
    if (!out_ || out_->spec != spec) {
        out_.reset();
        out_ = std::make_unique<Out>(spec, chain_, make_output_);
    }

    pipeline_ = std::make_unique<Pipeline>(std::move(source), *out_->output);
    out_->relay.point_at(pipeline_->decoded);

    track_ = ready_track_;
    ready_track_.clear();

    return true;
}

bool Session::reading() const noexcept
{
    return loader_->busy();
}

codec::OpenResult Session::open_result() const noexcept
{
    return last_result_;
}

const std::filesystem::path& Session::track() const noexcept
{
    return track_;
}

bool Session::loaded() const noexcept
{
    return pipeline_ != nullptr;
}

bool Session::play_or_pause()
{
    if (!loaded())
        return false;

    // The state says which of the three this means: silence it, carry on, or begin - the last of
    // which also covers playing a track that has already finished.
    switch (state()) {
    case Playback::State::playing:
        return pipeline_->player.pause();
    case Playback::State::paused:
        return pipeline_->player.resume();
    default:
        return pipeline_->player.start();
    }
}

void Session::stop() noexcept
{
    if (!loaded())
        return;

    pipeline_->player.stop();

    // Stopping is not pausing: the next play begins at the start of the track.
    pipeline_->player.seek(units::Time{});
}

void Session::seek(units::Time position) noexcept
{
    if (loaded())
        pipeline_->player.seek(position);
}

Playback::State Session::state() const noexcept
{
    return loaded() ? pipeline_->player.state() : Playback::State::idle;
}

bool Session::playing() const noexcept
{
    return loaded() && pipeline_->player.playing();
}

units::Time Session::time_played() const noexcept
{
    return loaded() ? pipeline_->player.time_played() : units::Time{};
}

units::Time Session::total_time() const noexcept
{
    return loaded() ? pipeline_->player.total_time() : units::Time{};
}

} // namespace wiola::engine
