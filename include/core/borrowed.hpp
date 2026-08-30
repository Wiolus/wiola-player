/**
 * @file
 * @brief A pointer to something owned elsewhere, which empties when it is handed on.
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

#include <core/macros.hpp>

#include <utility>

namespace wiola::core {

/**
 * A pointer to something owned elsewhere, held by whoever is allowed to use it.
 *
 * Moving one empties what it came from, so handing a thing to another thread leaves nothing
 * behind that can still reach it. That is the whole of it: the classes that hand out one end of
 * something are what say what an empty one does.
 */
template<typename T>
class Borrowed {
public:
    /// Empty, and so reaching nothing.
    Borrowed() noexcept = default;

    explicit Borrowed(T& target) noexcept
        : target_{&target}
    {
    }

    NO_COPY_SEMANTIC(Borrowed);

    Borrowed(Borrowed&& other) noexcept
        : target_{std::exchange(other.target_, nullptr)}
    {
    }

    Borrowed& operator=(Borrowed&& other) noexcept
    {
        target_ = std::exchange(other.target_, nullptr);

        return *this;
    }

    ~Borrowed() = default;

    /// Whether it reaches anything.
    [[nodiscard]] explicit operator bool() const noexcept { return target_ != nullptr; }

    [[nodiscard]] T* operator->() const noexcept { return target_; }

    [[nodiscard]] T& operator*() const noexcept { return *target_; }

private:
    T* target_{nullptr};
};

} // namespace wiola::core
