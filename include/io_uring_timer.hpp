//
// Copyright (c) 2023-present DeepGrace (complex dot invoke at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/deepgrace/unp
//

#ifndef IO_URING_TIMER_HPP
#define IO_URING_TIMER_HPP

namespace unp
{
    class timer_impl : timer_operation
    {
    public:
        void start() noexcept
        {
            if (context.is_running_on_io_thread())
                start_local();
            else
                start_remote();
        }

        static void on_schedule_complete(operation_base* op) noexcept
        {
            static_cast<timer_impl*>(op)->start_local();
        }

        static void on_timeout(operation_base* op) noexcept
        {
            auto& self = *static_cast<timer_impl*>(op);
            self.receiver(std::error_code());
        }

        static void on_cancel(operation_base* op) noexcept
        {
            auto& self = *static_cast<timer_impl*>(op);
            self.receiver(std::error_code(ECANCELED, std::system_category()));
        }

        static void remove_timer(operation_base* op) noexcept
        {
            auto& self = *static_cast<timer_impl*>(op);
            auto state = self.state.load(std::memory_order_relaxed);

            if ((state & timer_operation::timer_elapsed_flag) == 0)
                self.context.remove_timer(&self);

            self.receiver(std::error_code(ECANCELED, std::system_category()));
        }

        void start_local() noexcept
        {
            execute_ = &timer_impl::on_timeout;
            context.insert_timer(this);
        }

        void start_remote() noexcept
        {
            execute_ = &timer_impl::on_schedule_complete;
            context.schedule_remote(this);
        }

        void request_stop() noexcept
        {
            if (context.is_running_on_io_thread())
                request_stop_local();
            else
                request_stop_remote();
        }

        void request_stop_local() noexcept
        {
            assert(context.is_running_on_io_thread());

            execute_ = &timer_impl::on_cancel;
            auto state = this->state.load(std::memory_order_relaxed);

            if ((state & timer_operation::timer_elapsed_flag) == 0)
            {
                context.remove_timer(this);
                context.schedule_local(this);
            }
        }

        void request_stop_remote() noexcept
        {
            auto old_state = state.fetch_add(timer_operation::cancel_pending_flag, std::memory_order_acq_rel);

            if ((old_state & timer_operation::timer_elapsed_flag) == 0)
            {
                execute_ = &timer_impl::remove_timer;
                context.schedule_remote(this);
            }
        }

        struct cancel_callback
        {
            timer_impl& op;

            void operator()() noexcept
            {
                op.request_stop();
            }
        };

        explicit timer_impl(io_uring_context& context, const time_point& duetime) noexcept :
        timer_operation(context, duetime, true), context(context), duetime(duetime) 
        {
        }

        template <typename F>
        void async_wait(F&& f)
        {
            receiver = f;
            start();
        }

        io_uring_context& context;

        time_point duetime;
        std::function<void(std::error_code)> receiver;
    };

    class steady_timer
    {
    public:
        steady_timer(io_uring_context& context, const time_point& duetime = {}) : context(context), duetime(duetime)
        {
        }

        void expires_at(const time_point& tp)
        {
            duetime = tp;
        }

        template <typename Duration>
        void expires_after(const Duration& duration)
        {
            duetime = now() + duration;
        }

        template <typename Duration>
        void expires_from_now(const Duration& duration)
        {
            expires_after(duration);
        }

        time_point now() const noexcept
        {
            return monotonic_clock::now();
        }

        void cancel()
        {
            if (!context.timers.empty() && p)
                p->request_stop();
        }

        template <typename F>
        void async_wait(F&& f)
        {   
            cancel();

            p = std::make_shared<timer_impl>(context, duetime);
            p->async_wait([f, this](std::error_code ec){ f(ec); });
        }

        io_uring_context& context;

        time_point duetime;
        std::shared_ptr<timer_impl> p;
    };
}

#endif
