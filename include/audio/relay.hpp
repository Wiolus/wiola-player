/**
 * @file
 * @brief A source that plays whatever it has been pointed at.
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

#include <audio/source.hpp>
#include <audio/stream_spec.hpp>
#include <core/macros.hpp>

#include <atomic>
#include <cstddef>
#include <span>

namespace wiola::audio {

/**
 * A source that plays whatever it has been pointed at, so that a device can outlive a track.
 *
 * A device is bound to the source it plays for as long as it is open, which without this means
 * closing and reopening one whenever the track changes. Pointed at each track in turn, it lets
 * the device stay where it is.
 *
 * Pointing is for the thread that changes tracks; playing is for the device's. What it was
 * pointed at before has to outlive whatever may still be reading it, and stopping the device is
 * what makes that true: stopping waits for the callback to return, so a track let go after that
 * is a track nothing is reading.
 */
class Relay final : public Source {
public:
    explicit Relay(StreamSpec spec) noexcept;

    NO_COPY_SEMANTIC(Relay);
    NO_MOVE_SEMANTIC(Relay);

    ~Relay() override = default;

    /// Plays `source` from here on, which must be of the spec this was built for.
    void point_at(Source& source) noexcept;

    /// Plays nothing from here on.
    void point_at_nothing() noexcept;

    /// Whether it has anything to play.
    [[nodiscard]] bool pointed() const noexcept;

    /// What it is pointed at gives, and nothing when it is pointed at nothing: a device asking a
    /// relay with no track is asking for what is not there, and hears the silence it fills a
    /// short answer with.
    std::size_t render(std::span<float> interleaved) override;

    [[nodiscard]] StreamSpec spec() const noexcept override;

private:
    const StreamSpec spec_;
    std::atomic<Source*> source_{nullptr};
};

} // namespace wiola::audio
