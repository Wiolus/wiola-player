/**
 * @file
 * @brief Frequency and time quantities, convertible to a unit through to().
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

#include <cassert>
#include <chrono>
#include <compare>
#include <concepts>

namespace wiola::units {

namespace detail {

constexpr double pow10(int exponent) noexcept
{
    double result = 1.0;

    for (int i = 0; i < exponent; ++i) {
        result *= 10.0;
    }

    return result;
}

constexpr double from_base(double value, int exponent) noexcept
{
    return exponent < 0 ? value * pow10(-exponent) : value / pow10(exponent);
}

constexpr double to_base(double value, int exponent) noexcept
{
    return exponent < 0 ? value / pow10(-exponent) : value * pow10(exponent);
}

} // namespace detail

template<typename U, typename Q>
concept UnitOf = std::same_as<typename U::quantity, Q>;

/// Time, held in seconds.
class Time {
public:
    constexpr Time() noexcept = default;

    constexpr explicit Time(double seconds) noexcept
        : seconds_{seconds}
    {
    }

    template<typename Rep, typename Period>
    constexpr explicit Time(std::chrono::duration<Rep, Period> duration) noexcept
        : seconds_{std::chrono::duration<double>{duration}.count()}
    {
    }

    [[nodiscard]] constexpr double seconds() const noexcept { return seconds_; }

    [[nodiscard]] constexpr std::chrono::duration<double> chrono() const noexcept
    {
        return std::chrono::duration<double>{seconds_};
    }

    /// Value in time unit @p U.
    template<typename U>
        requires UnitOf<U, Time>
    [[nodiscard]] constexpr double get() const noexcept
    {
        return detail::from_base(seconds_, U::exp10);
    }

    /// Value in unit @p U. A frequency unit gives the rate, Frequency itself the rate as an object.
    template<typename U>
    [[nodiscard]] constexpr auto to() const noexcept;

    constexpr Time& operator+=(Time other) noexcept
    {
        seconds_ += other.seconds_;
        return *this;
    }

    constexpr Time& operator-=(Time other) noexcept
    {
        seconds_ -= other.seconds_;
        return *this;
    }

    constexpr Time& operator*=(double factor) noexcept
    {
        seconds_ *= factor;
        return *this;
    }

    constexpr Time& operator/=(double divisor) noexcept
    {
        assert(divisor != 0.0);
        seconds_ /= divisor;
        return *this;
    }

    friend constexpr auto operator<=>(Time, Time) = default;

    friend constexpr Time operator-(Time t) noexcept { return Time{-t.seconds_}; }

    friend constexpr Time operator+(Time a, Time b) noexcept
    {
        return Time{a.seconds_ + b.seconds_};
    }

    friend constexpr Time operator-(Time a, Time b) noexcept
    {
        return Time{a.seconds_ - b.seconds_};
    }

    /// Dimensionless ratio of two durations.
    friend constexpr double operator/(Time a, Time b) noexcept
    {
        assert(b.seconds_ != 0.0);
        return a.seconds_ / b.seconds_;
    }

    friend constexpr Time operator*(Time t, double factor) noexcept
    {
        return Time{t.seconds_ * factor};
    }

    friend constexpr Time operator*(double factor, Time t) noexcept
    {
        return Time{t.seconds_ * factor};
    }

    friend constexpr Time operator/(Time t, double divisor) noexcept
    {
        assert(divisor != 0.0);
        return Time{t.seconds_ / divisor};
    }

private:
    double seconds_{0.0};
};

/// Frequency, held in hertz.
class Frequency {
public:
    constexpr Frequency() noexcept = default;

    constexpr explicit Frequency(double hz) noexcept
        : hz_{hz}
    {
    }

    [[nodiscard]] constexpr double hz() const noexcept { return hz_; }

    /// Value in frequency unit @p U.
    template<typename U>
        requires UnitOf<U, Frequency>
    [[nodiscard]] constexpr double get() const noexcept
    {
        return detail::from_base(hz_, U::exp10);
    }

    /// Value in unit @p U. A time unit gives the period, Time itself the period as an object.
    template<typename U>
    [[nodiscard]] constexpr auto to() const noexcept
    {
        if constexpr (std::same_as<U, Time>) {
            assert(hz_ != 0.0);
            return Time{1.0 / hz_};
        } else if constexpr (UnitOf<U, Frequency>) {
            return detail::from_base(hz_, U::exp10);
        } else {
            static_assert(UnitOf<U, Time>, "not a frequency or time unit");
            assert(hz_ != 0.0);
            return detail::from_base(1.0 / hz_, U::exp10);
        }
    }

    constexpr Frequency& operator*=(double factor) noexcept
    {
        hz_ *= factor;
        return *this;
    }

    constexpr Frequency& operator/=(double divisor) noexcept
    {
        assert(divisor != 0.0);
        hz_ /= divisor;
        return *this;
    }

    friend constexpr auto operator<=>(Frequency, Frequency) = default;

    /// Dimensionless ratio of two frequencies.
    friend constexpr double operator/(Frequency a, Frequency b) noexcept
    {
        assert(b.hz_ != 0.0);
        return a.hz_ / b.hz_;
    }

    friend constexpr Frequency operator*(Frequency f, double factor) noexcept
    {
        return Frequency{f.hz_ * factor};
    }

    friend constexpr Frequency operator*(double factor, Frequency f) noexcept
    {
        return Frequency{f.hz_ * factor};
    }

    friend constexpr Frequency operator/(Frequency f, double divisor) noexcept
    {
        assert(divisor != 0.0);
        return Frequency{f.hz_ / divisor};
    }

    /// Cycles elapsed over @p t.
    friend constexpr double operator*(Frequency f, Time t) noexcept { return f.hz_ * t.seconds(); }

    friend constexpr double operator*(Time t, Frequency f) noexcept { return f.hz_ * t.seconds(); }

private:
    double hz_{0.0};
};

/// Unit tags accepted by to().
struct Hz {
    using quantity = Frequency;

    static constexpr int exp10 = 0;
};

struct KHz {
    using quantity = Frequency;

    static constexpr int exp10 = 3;
};

struct MHz {
    using quantity = Frequency;

    static constexpr int exp10 = 6;
};

struct Sec {
    using quantity = Time;

    static constexpr int exp10 = 0;
};

struct Ms {
    using quantity = Time;

    static constexpr int exp10 = -3;
};

struct Us {
    using quantity = Time;

    static constexpr int exp10 = -6;
};

struct Ns {
    using quantity = Time;

    static constexpr int exp10 = -9;
};

template<typename U>
constexpr auto Time::to() const noexcept
{
    if constexpr (std::same_as<U, Frequency>) {
        assert(seconds_ != 0.0);
        return Frequency{1.0 / seconds_};
    } else if constexpr (UnitOf<U, Time>) {
        return detail::from_base(seconds_, U::exp10);
    } else {
        static_assert(UnitOf<U, Frequency>, "not a frequency or time unit");
        assert(seconds_ != 0.0);
        return detail::from_base(1.0 / seconds_, U::exp10);
    }
}

inline namespace literals {

constexpr Frequency operator""_Hz(long double v) noexcept
{
    return Frequency{static_cast<double>(v)};
}

constexpr Frequency operator""_Hz(unsigned long long v) noexcept
{
    return Frequency{static_cast<double>(v)};
}

constexpr Frequency operator""_kHz(long double v) noexcept
{
    return Frequency{detail::to_base(static_cast<double>(v), KHz::exp10)};
}

constexpr Frequency operator""_kHz(unsigned long long v) noexcept
{
    return Frequency{detail::to_base(static_cast<double>(v), KHz::exp10)};
}

constexpr Frequency operator""_MHz(long double v) noexcept
{
    return Frequency{detail::to_base(static_cast<double>(v), MHz::exp10)};
}

constexpr Frequency operator""_MHz(unsigned long long v) noexcept
{
    return Frequency{detail::to_base(static_cast<double>(v), MHz::exp10)};
}

constexpr Time operator""_s(long double v) noexcept
{
    return Time{static_cast<double>(v)};
}

constexpr Time operator""_s(unsigned long long v) noexcept
{
    return Time{static_cast<double>(v)};
}

constexpr Time operator""_ms(long double v) noexcept
{
    return Time{detail::to_base(static_cast<double>(v), Ms::exp10)};
}

constexpr Time operator""_ms(unsigned long long v) noexcept
{
    return Time{detail::to_base(static_cast<double>(v), Ms::exp10)};
}

constexpr Time operator""_us(long double v) noexcept
{
    return Time{detail::to_base(static_cast<double>(v), Us::exp10)};
}

constexpr Time operator""_us(unsigned long long v) noexcept
{
    return Time{detail::to_base(static_cast<double>(v), Us::exp10)};
}

constexpr Time operator""_ns(long double v) noexcept
{
    return Time{detail::to_base(static_cast<double>(v), Ns::exp10)};
}

constexpr Time operator""_ns(unsigned long long v) noexcept
{
    return Time{detail::to_base(static_cast<double>(v), Ns::exp10)};
}

} // namespace literals

} // namespace wiola::units
