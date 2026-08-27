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

#include "tuning.hpp"

#include <audio/buffer_source.hpp>
#include <audio/device.hpp>
#include <audio/shaped_source.hpp>
#include <audio/stream_spec.hpp>
#include <codec/decoder.hpp>
#include <codec/open.hpp>
#include <lockfree/spsc_ring_buffer.hpp>

#include <utility>

namespace wiola::engine {

/// Declared in the order they are wired, so that each is built before whoever reads it and torn
/// down after.
struct Session::Pipeline {
    Pipeline(std::unique_ptr<codec::Decoder> source, audio::Chain& chain,
        const OutputFactory& make_output)
        : buffer{source->spec().samples_per(tuning::buffer_duration)}
        , decoded{source->spec(), buffer}
        , shaped{decoded, chain}
        , output{make_output(shaped)}
        , player{std::move(source), buffer, *output}
    {
    }

    lockfree::SPSCRingBuffer<float> buffer;
    audio::BufferSource decoded;
    audio::ShapedSource shaped;
    std::unique_ptr<audio::Output> output;
    Player player;
};

Session::Session()
    : Session{[](audio::Source& source) { return std::make_unique<audio::Device>(source); }}
{
}

Session::Session(OutputFactory make_output)
    : make_output_{std::move(make_output)}
{
}

Session::~Session() = default;

codec::OpenResult Session::load(const std::filesystem::path& path)
{
    codec::Opened opened{codec::open_file(path)};

    if (!opened) {
        pipeline_.reset();
        return opened.result;
    }

    std::unique_ptr<codec::Decoder> source{std::move(opened.decoder)};
    const audio::StreamSpec spec{source->spec()};

    // The old pipeline goes first: its device reads the chain that configuring one rebuilds.
    pipeline_.reset();
    chain_.configure(spec);
    pipeline_ = std::make_unique<Pipeline>(std::move(source), chain_, make_output_);

    return codec::OpenResult::opened;
}

bool Session::loaded() const noexcept
{
    return pipeline_ != nullptr;
}

bool Session::toggle()
{
    if (!loaded())
        return false;

    // The state says which of the three this means: silence it, carry on, or begin - the last of
    // which also covers playing a track that has already finished.
    switch (state()) {
    case PlayerState::playing:
        return pipeline_->player.pause();
    case PlayerState::paused:
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

PlayerState Session::state() const noexcept
{
    return loaded() ? pipeline_->player.state() : PlayerState::idle;
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
