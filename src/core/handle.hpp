/**
 * @file
 * @brief A capability handed out once, and the end that holds it.
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

#include <core/borrowed.hpp>
#include <core/macros.hpp>

namespace wiola::core {

/**
 * Whether the one thing a class hands out has been taken.
 *
 * Some of what an object can do belongs to one thread at a time: driving a device, carrying seeks
 * out, filling a buffer. Rather than say so in a comment, the object hands that end out once, and
 * this is what remembers that it has.
 */
class Claimable {
public:
    /// Claims it for this caller. True for the first caller only; every one after gets false.
    [[nodiscard]] bool claim() noexcept
    {
        if (claimed_)
            return false;

        claimed_ = true;

        return true;
    }

    /// Free to be claimed again. For whoever hands it out, never for whoever holds it.
    void release() noexcept { claimed_ = false; }

    [[nodiscard]] bool claimed() const noexcept { return claimed_; }

private:
    bool claimed_{false};
};

/**
 * The end of `Owner` that was handed out, held by whoever may use it.
 *
 * Moving one hands that end to somebody else and leaves nothing behind: what is moved from holds
 * nothing, and neither does one taken after the first. Holding nothing is not an error - a
 * handle that reaches nothing does nothing - so what each of them does with that is theirs to
 * say, which is why the calls a handle forwards are written on the handle itself.
 */
template<typename Owner>
class Handle {
public:
    NO_COPY_SEMANTIC(Handle);
    DEFAULT_MOVE_SEMANTIC(Handle);

    ~Handle() = default;

    /// Whether it reaches what it was handed out for.
    [[nodiscard]] explicit operator bool() const noexcept { return static_cast<bool>(owner_); }

protected:
    /// Reaching nothing, which is what a second taker gets.
    Handle() noexcept = default;

    explicit Handle(Owner& target) noexcept
        : owner_{target}
    {
    }

    /// What it reaches, for the calls it forwards. Empty when it reaches nothing.
    [[nodiscard]] const Borrowed<Owner>& owner() const noexcept { return owner_; }

    /// Reaches nothing from here on. For a handle that returns its end when it is destroyed.
    void clear() noexcept { owner_ = Borrowed<Owner>{}; }

private:
    Borrowed<Owner> owner_;
};

} // namespace wiola::core
