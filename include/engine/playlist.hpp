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

#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <vector>

namespace wiola::engine {

/**
 * The tracks to be played, in the order they are to be played in, and where in that playback has
 * reached.
 *
 * It holds names and a place, and plays nothing itself: what a track sounds like, and whether it
 * can be opened at all, is settled elsewhere. That is what makes it answerable on its own.
 *
 * Shuffling is an order, not a choice made afresh each time a track ends, so that going back is
 * going back to what was heard rather than to somewhere new.
 */
class Playlist {
public:
    /// What to do when the last track of the order runs out.
    enum class Repeat {
        /// Nothing: playback has reached the end.
        none,

        /// The track that just played, again.
        track,

        /// The first track of the order, again.
        all,
    };

    /// Replaces everything with `tracks`, and stands at the first of them.
    void set(std::vector<std::filesystem::path> tracks);

    /// Puts `track` at the end. A list that was empty stands at it.
    void add(std::filesystem::path track);

    /// Empties it, leaving it standing nowhere.
    void clear() noexcept;

    [[nodiscard]] bool empty() const noexcept;
    [[nodiscard]] std::size_t size() const noexcept;

    /// Every track, in the order they were given. Shuffling changes what plays next, not what
    /// this says: a listener looking at their list wants to see the list they made.
    [[nodiscard]] const std::vector<std::filesystem::path>& tracks() const noexcept;

    /// The track it stands at, or nothing when it stands nowhere.
    [[nodiscard]] const std::filesystem::path& current() const noexcept;

    /// Where it stands, counted along the list as it was given, or nothing when it stands
    /// nowhere. Shuffling changes what comes next, not what a track is called or where it was
    /// put.
    [[nodiscard]] std::optional<std::size_t> position() const noexcept;

    /// Stands at the track that was put at `index`. False when there is no such track.
    bool go_to(std::size_t index) noexcept;

    /// Stands at the next track of the order, or wherever `repeat` says instead. False when
    /// there is nowhere to go, which is what the end of playback is.
    bool next() noexcept;

    /// Stands at the one before, by the same rules.
    bool previous() noexcept;

    void set_repeat(Repeat repeat) noexcept;
    [[nodiscard]] Repeat repeat() const noexcept;

    /// Puts the tracks in an order drawn from `seed`, keeping the one it stands at. The same
    /// seed gives the same order, so what a listener heard can be played again.
    void shuffle(std::uint_fast32_t seed);

    /// Puts them back in the order they were given, keeping the one it stands at.
    void unshuffle();

    [[nodiscard]] bool shuffled() const noexcept;

private:
    /// Where in `order_` the track put at `index` now sits.
    [[nodiscard]] std::size_t place_of(std::size_t index) const noexcept;

    std::vector<std::filesystem::path> tracks_;

    /// The order they are played in, as places in `tracks_`.
    std::vector<std::size_t> order_;

    /// Where in `order_` it stands, or nothing when it stands nowhere.
    std::optional<std::size_t> place_;

    Repeat repeat_{Repeat::none};
    bool shuffled_{false};
};

} // namespace wiola::engine
