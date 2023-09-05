//
// Copyright (c) 2023-present DeepGrace (complex dot invoke at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/deepgrace/unp
//

#ifndef IO_URING_BASE_HPP
#define IO_URING_BASE_HPP

#include <atomic>
#include <linux/io_uring.h>
#include <monotonic_clock.hpp>

namespace unp
{
    using offset_t = std::int64_t;
    using time_point = monotonic_clock::time_point;

    static constexpr __u64 remote_queue_event_user_data = 0;

    struct __kernel_timespec
    {
        int64_t tv_sec;
        long long tv_nsec;
    };

    struct operation_base
    {
        operation_base() noexcept
        {
        }

        operation_base* next;

        void (*execute_)(operation_base*) noexcept;
    };

    struct completion_base : operation_base
    {
        int result;
    };

    struct stop_operation : operation_base
    {
        stop_operation() noexcept
        {
            execute_ = [](operation_base* op) noexcept
            {
                static_cast<stop_operation*>(op)->should_stop = true;
            };
        }

        bool should_stop = false;
    };

    class io_uring_context;
    static thread_local io_uring_context* current_thread_context;

    struct timer_operation : operation_base
    {
        explicit timer_operation(io_uring_context& context, const time_point& duetime, bool cancelable) noexcept :
        context(context), duetime(duetime), cancelable(cancelable)
        {
        }

        io_uring_context& context;

        time_point duetime;
        bool cancelable;

        timer_operation* next;
        timer_operation* prev;

        static constexpr std::uint32_t timer_elapsed_flag = 1;
        static constexpr std::uint32_t cancel_pending_flag = 2;

        std::atomic<std::uint32_t> state = 0;
    };
}

#endif
