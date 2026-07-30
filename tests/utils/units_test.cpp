/**
 * @file
 * @brief Tests for frequency and time quantities.
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

#include <utils/units.hpp>

#include <gtest/gtest.h>

#include <chrono>
#include <type_traits>

using namespace wiola::units;

namespace {

template<typename A, typename B>
concept Addable = requires(A a, B b) { a + b; };

template<typename A, typename B>
concept Subtractable = requires(A a, B b) { a - b; };

template<typename A, typename B>
concept Multipliable = requires(A a, B b) { a * b; };

template<typename A, typename B>
concept Divisible = requires(A a, B b) { a / b; };

template<typename A>
concept Negatable = requires(A a) { -a; };

template<typename Q, typename U>
concept Gettable = requires(Q q) { q.template get<U>(); };

template<typename A>
concept DefaultConstructible = requires { A{}; };

} // namespace

TEST(Units, ReadsFrequencyInEveryUnit)
{
    constexpr Frequency rate = 48_kHz;

    static_assert(rate.to<Hz>() == 48000.0);
    static_assert(rate.to<KHz>() == 48.0);
    static_assert((1_MHz).to<KHz>() == 1000.0);
    static_assert((1_MHz).to<Hz>() == 1000000.0);
}

TEST(Units, ReadsTimeInEveryUnit)
{
    constexpr Time interval = 1.5_s;

    static_assert(interval.to<Sec>() == 1.5);
    static_assert(interval.to<Ms>() == 1500.0);
    static_assert((2_ms).to<Us>() == 2000.0);
    static_assert((1_us).to<Ns>() == 1000.0);
}

TEST(Units, ConvertsFrequencyToTimeAndBack)
{
    static_assert((1_kHz).to<Time>() == 1_ms);
    static_assert((1_MHz).to<Time>() == 1_us);
    static_assert((1_ms).to<Frequency>() == 1_kHz);
    static_assert((1_us).to<Frequency>() == 1_MHz);
    static_assert((100_Hz).to<Ms>() == 10.0);
    static_assert((1_ms).to<KHz>() == 1.0);
    static_assert((1_us).to<MHz>() == 1.0);
    static_assert((250_ms).to<Hz>() == 4.0);
}

TEST(Units, InvertsScaleBetweenHertzAndMilliseconds)
{
    static_assert((1_Hz).to<Time>() == 1_s);
    static_assert((1_Hz).to<Sec>() == 1.0);
    static_assert((1_Hz).to<Ms>() == 1000.0);
    static_assert((1_Hz).to<Us>() == 1000000.0);

    static_assert((1_kHz).to<Ms>() == 1.0);
    static_assert((1_s).to<Hz>() == 1.0);
    static_assert((1000_ms).to<Hz>() == 1.0);
    static_assert((1_ms).to<Hz>() == 1000.0);
}

TEST(Units, RoundTripsThroughTheOtherQuantity)
{
    EXPECT_DOUBLE_EQ((440_Hz).to<Time>().to<Frequency>().hz(), 440.0);
    EXPECT_DOUBLE_EQ((20_ms).to<Frequency>().to<Ms>(), 20.0);
}

TEST(Units, ComparesAndAddsWithinAQuantity)
{
    static_assert(1000_ms == 1_s);
    static_assert(1_kHz == 1000_Hz);
    static_assert(1_MHz > 999_kHz);
    static_assert(1_us < 1_ms);
    static_assert((1_s + 500_ms) == 1500_ms);
    static_assert((1_s - 250_ms) == 750_ms);
}

TEST(Units, MultipliesFrequencyByTimeIntoCycles)
{
    static_assert(440_Hz * 2_s == 880.0);
    static_assert(1_kHz * 3_ms == 3.0);
    static_assert(std::is_same_v<decltype(440_Hz * 2_s), double>);
}

TEST(Units, RejectsAdditiveOperationsOnFrequency)
{
    static_assert(!Addable<Frequency, Frequency>);
    static_assert(!Subtractable<Frequency, Frequency>);
    static_assert(Multipliable<Frequency, double>);
    static_assert(Divisible<Frequency, double>);
    static_assert(Multipliable<Frequency, Time>);

    static_assert(Addable<Time, Time>);
    static_assert(Subtractable<Time, Time>);
}

TEST(Units, NegatesTime)
{
    static_assert(-1_s == Time{-1.0});
    static_assert(-(-1_s) == 1_s);
    static_assert((-250_ms).to<Ms>() == -250.0);
    static_assert(1_s + -250_ms == 750_ms);
    static_assert(!Negatable<Frequency>);
}

TEST(Units, AccumulatesInPlace)
{
    Time elapsed{0.0};
    elapsed += 250_ms;
    elapsed += 250_ms;
    EXPECT_DOUBLE_EQ(elapsed.to<Ms>(), 500.0);

    elapsed -= 100_ms;
    EXPECT_DOUBLE_EQ(elapsed.to<Ms>(), 400.0);

    elapsed *= 2.0;
    EXPECT_DOUBLE_EQ(elapsed.to<Ms>(), 800.0);

    elapsed /= 4.0;
    EXPECT_DOUBLE_EQ(elapsed.to<Ms>(), 200.0);

    Frequency tone = 440_Hz;
    tone *= 2.0;
    EXPECT_DOUBLE_EQ(tone.hz(), 880.0);

    tone /= 4.0;
    EXPECT_DOUBLE_EQ(tone.hz(), 220.0);
}

TEST(Units, DividesLikeQuantitiesIntoARatio)
{
    static_assert(1_s / 250_ms == 4.0);
    static_assert(1_kHz / 440_Hz == 1000.0 / 440.0);
    static_assert(std::is_same_v<decltype(1_s / 250_ms), double>);
    static_assert(std::is_same_v<decltype(1_kHz / 440_Hz), double>);

    static_assert(std::is_same_v<decltype(1_s / 4.0), Time>);
    static_assert(std::is_same_v<decltype(440_Hz / 2.0), Frequency>);

    static_assert(!Addable<Frequency, Frequency>);
}

TEST(Units, ReadsItsOwnUnitsWithGet)
{
    static_assert((48_kHz).get<Hz>() == 48000.0);
    static_assert((48_kHz).get<KHz>() == 48.0);
    static_assert((1_MHz).get<KHz>() == 1000.0);

    static_assert((1.5_s).get<Sec>() == 1.5);
    static_assert((1.5_s).get<Ms>() == 1500.0);
    static_assert((2_ms).get<Us>() == 2000.0);
}

TEST(Units, RefusesTheOtherQuantityInGet)
{
    static_assert(!Gettable<Time, Hz>);
    static_assert(!Gettable<Time, KHz>);
    static_assert(!Gettable<Frequency, Ms>);
    static_assert(!Gettable<Frequency, Sec>);

    static_assert(Gettable<Time, Ms>);
    static_assert(Gettable<Frequency, Hz>);
}

TEST(Units, ConvertsToAndFromChrono)
{
    using namespace std::chrono_literals;

    static_assert(Time{250ms}.get<Ms>() == 250.0);
    static_assert(Time{1s}.get<Sec>() == 1.0);
    static_assert(Time{1500us}.get<Ms>() == 1.5);

    static_assert((250_ms).chrono() == std::chrono::duration<double>{0.25});

    constexpr auto deadline = std::chrono::duration<double>{2.0};
    static_assert(Time{deadline} == 2_s);
}

TEST(Units, DefaultsToZero)
{
    static_assert(DefaultConstructible<Time>);
    static_assert(DefaultConstructible<Frequency>);

    static_assert(Time{}.get<Sec>() == 0.0);
    static_assert(Time{}.get<Ms>() == 0.0);
    static_assert(Time{} == 0_s);

    static_assert(Frequency{}.hz() == 0.0);
    static_assert(Frequency{}.get<KHz>() == 0.0);
    static_assert(Frequency{} == 0_Hz);

    static_assert((Time{} + 250_ms) == 250_ms);
}

TEST(Units, BuildsFromIntegers)
{
    constexpr std::size_t rate{48000};
    constexpr int count{440};

    static_assert(Frequency{rate} == 48_kHz);
    static_assert(Frequency{count} == 440_Hz);
    static_assert(Time{std::size_t{2}} == 2_s);
    static_assert(Time{2} == 2_s);

    static_assert(Frequency{440.5} == Frequency{440.5});
}

TEST(Units, ScalesByAPlainNumber)
{
    static_assert(2.0 * 440_Hz == 880_Hz);
    static_assert(1_s / 4.0 == 250_ms);
}
