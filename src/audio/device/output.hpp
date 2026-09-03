/**
 * @file
 * @brief Where playback runs.
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

#include <core/handle.hpp>
#include <core/macros.hpp>
#include <pcm/stream_spec.hpp>

#include <utility>

#include <cstddef>

namespace wiola::audio {

/// Somewhere frames are played, which is started and stopped and says how much has been heard.
class Output {
public:
    /**
     * The end that starts and stops it, for the one thread that drives the device.
     *
     * Handed out rather than reachable from the output itself, so that a thread which only asks
     * what has been played cannot start or stop what another thread is driving: two threads
     * doing so leaves the device in a state neither of them asked for, and a device is not a
     * thing that can be asked which it is in.
     *
     * Moving it hands the device to another thread. What is moved from, and any control taken
     * while another is still held, does nothing at all.
     *
     * Letting one go gives the device back, so the next thing to drive it can take one: a device
     * outlives the tracks played through it, and each of them drives it in turn. Taking and
     * giving back are for whoever owns the device, never for the thread that plays.
     */
    class Control final : public core::Handle<Output> {
    public:
        Control(Control&& other) noexcept = default;

        Control& operator=(Control&& other) noexcept
        {
            if (this != &other) {
                release();
                Handle::operator=(std::move(other));
            }

            return *this;
        }

        /// Gives the device back, so that something else may drive it.
        ~Control() { release(); }

        /// Begins playing what the output pulls. False when there is none to be had.
        [[nodiscard]] bool start() noexcept;

        /// Halts playing. Starting again is the caller's to ask for.
        void stop() noexcept;

    private:
        friend class Output;

        using Handle::Handle;

        /// Gives back whatever it holds, and holds nothing afterwards.
        void release() noexcept
        {
            if (owner())
                owner()->control_taken_.release();

            clear();
        }
    };

    NO_COPY_SEMANTIC(Output);
    NO_MOVE_SEMANTIC(Output);

    virtual ~Output() = default;

    /// The end that drives it. There is one at a time: while a control is held, another taken is
    /// inert, so a second driver is a device that will not start rather than two threads starting
    /// one. Letting the held one go frees the device for the next.
    [[nodiscard]] Control control() noexcept;

    [[nodiscard]] virtual bool running() const noexcept = 0;

    /// Frames of the source that have been played. Silence emitted because the source had nothing
    /// to give does not count: this measures material, not elapsed time, so a position taken from
    /// it stands still while a source is starved instead of running on without it.
    [[nodiscard]] virtual pcm::Frames frames_played() const noexcept = 0;

protected:
    Output() = default;

private:
    /// Driving it is the control's, above. Overridden here, called only from there.
    [[nodiscard]] virtual bool start() noexcept = 0;
    virtual void stop() noexcept = 0;

    core::Claimable control_taken_;
};

inline bool Output::Control::start() noexcept
{
    return owner() && owner()->start();
}

inline void Output::Control::stop() noexcept
{
    if (owner())
        owner()->stop();
}

inline Output::Control Output::control() noexcept
{
    return control_taken_.claim() ? Control{*this} : Control{};
}

} // namespace wiola::audio
