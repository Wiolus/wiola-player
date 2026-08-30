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

#include <audio/stream_spec.hpp>
#include <core/macros.hpp>

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
     * after the first, does nothing at all.
     */
    class Control {
    public:
        NO_COPY_SEMANTIC(Control);

        Control(Control&& other) noexcept
            : output_{std::exchange(other.output_, nullptr)}
        {
        }

        Control& operator=(Control&& other) noexcept
        {
            output_ = std::exchange(other.output_, nullptr);

            return *this;
        }

        ~Control() = default;

        /// Begins playing what the output pulls. False when there is none to be had.
        [[nodiscard]] bool start() noexcept { return output_ != nullptr && output_->start(); }

        /// Halts playing. Starting again is the caller's to ask for.
        void stop() noexcept
        {
            if (output_ != nullptr)
                output_->stop();
        }

    private:
        friend class Output;

        Control() noexcept = default;

        explicit Control(Output& output) noexcept
            : output_{&output}
        {
        }

        Output* output_{nullptr};
    };

    NO_COPY_SEMANTIC(Output);
    NO_MOVE_SEMANTIC(Output);

    virtual ~Output() = default;

    /// The end that drives it. There is one: whoever takes it first has the device, and every
    /// one taken after that is inert, so a second driver is a device that will not start rather
    /// than two threads starting one.
    [[nodiscard]] Control control() noexcept
    {
        if (control_taken_)
            return Control{};

        control_taken_ = true;

        return Control{*this};
    }

    [[nodiscard]] virtual bool running() const noexcept = 0;

    /// Frames played, which is what has been heard.
    [[nodiscard]] virtual Frames frames_played() const noexcept = 0;

protected:
    Output() = default;

private:
    /// Driving it is the control's, above. Overridden here, called only from there.
    [[nodiscard]] virtual bool start() noexcept = 0;
    virtual void stop() noexcept = 0;

    bool control_taken_{false};
};

} // namespace wiola::audio
