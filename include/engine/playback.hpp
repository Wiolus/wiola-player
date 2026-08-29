/**
 * @file
 * @brief What playback is doing, and the rules by which that changes.
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

#include <atomic>

namespace wiola::engine {

/**
 * What playback is doing, and every way that may change.
 *
 * Every transition is one atomic step, so any thread may ask for any of them at any moment and
 * no ask waits for another. Which threads ask, and so which asks meet:
 *
 * - a listener's thread pauses, resumes and stops;
 * - the thread that decodes ends a track it has played out, and stops one whose output failed.
 *
 * Two pairs therefore land together in practice - a pause or a resume against the end of a
 * track, and a stop against the same end - and the rules below settle both, rather than
 * whichever store happens to land last. `begin` is the exception: it is asked for only where no
 * decoding thread exists, so it meets nothing.
 *
 * The state is all this holds. What a transition means for a device is for whoever owns one.
 */
class Playback {
public:
    /**
     * What playback is doing.
     *
     * This is decided here rather than read back from the device. The device can stop on its
     * own - an output can be lost, or an audio server can give up on a stream - and that is a
     * fault to be noticed, not a change of what the listener asked for.
     *
     * `ended` and `stopped` are both final and differ in why: a source that ran out is the cue
     * to play the next thing, while a listener who pressed stop is not.
     */
    enum class State {
        idle,
        playing,
        paused,
        ended,
        stopped,
    };

    [[nodiscard]] State state() const noexcept;

    /// Whether a track is playing. Says nothing about whether a device agrees.
    [[nodiscard]] bool playing() const noexcept;

    /// Whether playback is over, however it ended. `state()` says which.
    [[nodiscard]] bool finished() const noexcept;

    /// Begins playing, from idle or from playback that has already finished: playing a track
    /// again is this same call. False when playback is already under way.
    bool begin() noexcept;

    /// Playing to paused. False when there was nothing playing to pause, which a track that
    /// ended first counts as.
    bool pause() noexcept;

    /// Paused to playing. False unless playback is paused: resuming is not a way to begin.
    bool resume() noexcept;

    /// Settles on `reason`, which must be `ended` or `stopped`. The first final state stays,
    /// whichever thread reached it: a listener's stop is not undone by the source running out a
    /// moment later, nor the other way about.
    void finish(State reason) noexcept;

private:
    std::atomic<State> state_{State::idle};
};

} // namespace wiola::engine
