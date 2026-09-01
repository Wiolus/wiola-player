/**
 * @file
 * @brief What is to be played, and where in it playback has reached.
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

#include <engine/playlist.hpp>

#include <algorithm>
#include <numeric>
#include <random>
#include <utility>

namespace wiola::engine {

namespace {

/// What `current` answers with when it stands nowhere.
const Track& nothing()
{
    static const Track empty;

    return empty;
}

} // namespace

void Playlist::set(std::vector<Track> tracks)
{
    tracks_ = std::move(tracks);

    order_.resize(tracks_.size());
    std::iota(order_.begin(), order_.end(), std::size_t{0});

    shuffled_ = false;
    place_ = tracks_.empty() ? std::nullopt : std::optional<std::size_t>{0};
}

void Playlist::add(Track track)
{
    tracks_.push_back(std::move(track));

    // A track added to a shuffled list goes at the end of the order rather than into the middle
    // of it: where it lands is the listener's to decide by shuffling again.
    order_.push_back(tracks_.size() - 1);

    if (!place_.has_value())
        place_ = 0;
}

bool Playlist::remove(std::size_t index)
{
    if (index >= tracks_.size())
        return false;

    const std::optional<std::size_t> standing{position()};

    tracks_.erase(tracks_.begin() + static_cast<std::ptrdiff_t>(index));

    // The order holds places in the list, so the place taken out goes, and every place after it
    // has moved down one.
    order_.erase(std::ranges::find(order_, index));

    for (std::size_t& place : order_)
        if (place > index)
            --place;

    if (tracks_.empty()) {
        place_.reset();

        return true;
    }

    if (!standing.has_value()) {
        place_ = 0;

        return true;
    }

    // Taking out the one it stands at leaves it standing at whatever followed, which is now at
    // the same place - unless that was the end of the list.
    if (*standing == index) {
        place_ = place_of(std::min(index, tracks_.size() - 1));

        return true;
    }

    place_ = place_of(*standing > index ? *standing - 1 : *standing);

    return true;
}

void Playlist::clear() noexcept
{
    tracks_.clear();
    order_.clear();
    place_.reset();
    shuffled_ = false;
}

bool Playlist::empty() const noexcept
{
    return tracks_.empty();
}

std::size_t Playlist::size() const noexcept
{
    return tracks_.size();
}

const std::vector<Track>& Playlist::tracks() const noexcept
{
    return tracks_;
}

const Track& Playlist::current() const noexcept
{
    if (!place_.has_value())
        return nothing();

    return tracks_[order_[*place_]];
}

bool Playlist::describe(std::size_t index, Track track)
{
    if (index >= tracks_.size())
        return false;

    tracks_[index] = std::move(track);

    return true;
}

std::optional<std::size_t> Playlist::position() const noexcept
{
    if (!place_.has_value())
        return std::nullopt;

    return order_[*place_];
}

bool Playlist::go_to(std::size_t index) noexcept
{
    if (index >= tracks_.size())
        return false;

    place_ = place_of(index);

    return true;
}

bool Playlist::next() noexcept
{
    if (!place_.has_value())
        return false;

    if (repeat_ == Repeat::track)
        return true;

    if (*place_ + 1 < order_.size()) {
        place_ = *place_ + 1;

        return true;
    }

    if (repeat_ != Repeat::all)
        return false;

    place_ = 0;

    return true;
}

bool Playlist::previous() noexcept
{
    if (!place_.has_value())
        return false;

    if (repeat_ == Repeat::track)
        return true;

    if (*place_ > 0) {
        place_ = *place_ - 1;

        return true;
    }

    if (repeat_ != Repeat::all)
        return false;

    place_ = order_.size() - 1;

    return true;
}

void Playlist::set_repeat(Repeat repeat) noexcept
{
    repeat_ = repeat;
}

Playlist::Repeat Playlist::repeat() const noexcept
{
    return repeat_;
}

void Playlist::shuffle(std::uint_fast32_t seed)
{
    const std::optional<std::size_t> standing{position()};

    std::mt19937 random{seed};
    std::ranges::shuffle(order_, random);

    shuffled_ = true;

    // The track being played is still the track being played: only what comes after it changed.
    if (standing.has_value())
        place_ = place_of(*standing);
}

void Playlist::unshuffle()
{
    const std::optional<std::size_t> standing{position()};

    std::iota(order_.begin(), order_.end(), std::size_t{0});
    shuffled_ = false;

    if (standing.has_value())
        place_ = place_of(*standing);
}

bool Playlist::shuffled() const noexcept
{
    return shuffled_;
}

std::size_t Playlist::place_of(std::size_t index) const noexcept
{
    const auto found = std::ranges::find(order_, index);

    return static_cast<std::size_t>(std::distance(order_.begin(), found));
}

} // namespace wiola::engine
