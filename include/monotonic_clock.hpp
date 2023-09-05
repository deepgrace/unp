//
// Copyright (c) 2023-present DeepGrace (complex dot invoke at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/deepgrace/unp
//

#ifndef MONOTONIC_CLOCK_HPP
#define MONOTONIC_CLOCK_HPP

#include <ratio>
#include <chrono>
#include <limits>

namespace unp
{
    class monotonic_clock
    {
    public:
        using rep = std::int64_t;
        using ratio = std::ratio<1, 10'000'000>;

        static constexpr bool is_steady = true;
        using duration = std::chrono::duration<rep, ratio>;

        class time_point
        {
        public:
            using duration = monotonic_clock::duration;

            constexpr time_point() noexcept
            {
            }

            constexpr time_point(const time_point&) noexcept = default;
            time_point& operator=(const time_point&) noexcept = default;

            static constexpr time_point min() noexcept
            {
                time_point tp;

                tp.seconds_ = std::numeric_limits<std::int64_t>::min();
                tp.nanoseconds_ = -999'999'999;

                return tp;
            }

            static constexpr time_point max() noexcept
            {
                time_point tp;

                tp.seconds_ = std::numeric_limits<std::int64_t>::max();
                tp.nanoseconds_ = 999'999'999;

                return tp;
            }

            static time_point from_seconds_and_nanoseconds(std::int64_t seconds, long long nanoseconds) noexcept
            {
                time_point tp;

                tp.seconds_ = seconds;
                tp.nanoseconds_ = nanoseconds;

                tp.normalize();

                return tp;
            }

            constexpr std::int64_t seconds_part() const noexcept
            {
                return seconds_;
            }

            constexpr long long nanoseconds_part() const noexcept
            {
                return nanoseconds_;
            }

            template <typename Rep, typename Ratio>
            time_point& operator+=(const std::chrono::duration<Rep, Ratio>& d) noexcept
            {
                const auto whole_seconds = std::chrono::duration_cast<std::chrono::seconds>(d);
                const auto remainder_nanoseconds = std::chrono::duration_cast<std::chrono::nanoseconds>(d - whole_seconds);

                seconds_ += whole_seconds.count();
                nanoseconds_ += remainder_nanoseconds.count();

                normalize();

                return *this;
            }

            template <typename Rep, typename Ratio>
            time_point& operator-=(const std::chrono::duration<Rep, Ratio>& d) noexcept
            {
                const auto whole_seconds = std::chrono::duration_cast<std::chrono::seconds>(d);
                const auto remainder_nanoseconds =std::chrono::duration_cast<std::chrono::nanoseconds>(d - whole_seconds);

                seconds_ -= whole_seconds.count();
                nanoseconds_ -= remainder_nanoseconds.count();

                normalize();

                return *this;
            }

            friend duration operator-(const time_point& a, const time_point& b) noexcept
            {
                return duration((a.seconds_ - b.seconds_) * 10'000'000 + (a.nanoseconds_ - b.nanoseconds_) / 100);
            }

            template <typename Rep, typename Ratio>
            friend time_point operator+(const time_point& a, std::chrono::duration<Rep, Ratio> d) noexcept
            {
                time_point tp = a;
                tp += d;

                return tp;
            }

            template <typename Rep, typename Ratio>
            friend time_point operator-(const time_point& a, std::chrono::duration<Rep, Ratio> d) noexcept
            {
                time_point tp = a;
                tp -= d;

                return tp;
            }

            friend bool operator==(const time_point& a, const time_point& b) noexcept
            {
                return a.seconds_ == b.seconds_ && a.nanoseconds_ == b.nanoseconds_;
            }

            friend bool operator!=(const time_point& a, const time_point& b) noexcept
            {
                return !(a == b);
            }

            friend bool operator<(const time_point& a, const time_point& b) noexcept
            {
                return (a.seconds_ == b.seconds_) ? (a.nanoseconds_ < b.nanoseconds_) : (a.seconds_ < b.seconds_);
            }

            friend bool operator>(const time_point& a, const time_point& b) noexcept
            {
                return b < a;
            }

            friend bool operator<=(const time_point& a, const time_point& b) noexcept
            {
                return !(b < a);
            }

            friend bool operator>=(const time_point& a, const time_point& b) noexcept
            {
                return !(a < b);
            }

        private:
            void normalize() noexcept
            {
                constexpr std::int64_t nanoseconds_per_second = 1'000'000'000;
                auto extra_seconds = nanoseconds_ / nanoseconds_per_second;

                seconds_ += extra_seconds;
                nanoseconds_ -= extra_seconds * nanoseconds_per_second;

                if (seconds_ < 0 && nanoseconds_ > 0)
                {
                    seconds_ += 1;
                    nanoseconds_ -= nanoseconds_per_second;
                }
                else if (seconds_ > 0 && nanoseconds_ < 0)
                {
                    seconds_ -= 1;
                    nanoseconds_ += nanoseconds_per_second;
                }
            }

            std::int64_t seconds_ = 0;
            long long nanoseconds_ = 0;
        };

        static time_point now() noexcept
        {
            timespec ts;
            clock_gettime(CLOCK_MONOTONIC, &ts);

            return time_point::from_seconds_and_nanoseconds(ts.tv_sec, ts.tv_nsec);
        }
    };
}

#endif
