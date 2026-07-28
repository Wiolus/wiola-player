/**
 * @file
 * @brief Cache line size used to keep independently written data off a shared line.
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
 * along with Wiola.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <cstddef>
#include <new>

namespace wiola::hw {

// The standard constant is not always there: clang + libstdc++ has no
// std::hardware_destructive_interference_size, hence the fallback.
#if defined(__cpp_lib_hardware_interference_size)
// A copy rather than a using-declaration keeps GCC's -Winterference-size, which it raises
// wherever the value reaches an ABI, on this line instead of at every use site.
inline constexpr std::size_t hardware_destructive_interference_size =
    std::hardware_destructive_interference_size;
#else
inline constexpr std::size_t hardware_destructive_interference_size = 64; // x86-64, AArch64
#endif

} // namespace wiola::hw
