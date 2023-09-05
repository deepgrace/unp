//
// Copyright (c) 2023-present DeepGrace (complex dot invoke at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/deepgrace/unp
//

#ifndef INPLACE_STOP_TOKEN_HPP
#define INPLACE_STOP_TOKEN_HPP

#include <atomic>
#include <thread>
#include <cassert>
#include <cstdint>
#include <type_traits>
#include <spin_wait.hpp>

namespace unp
{
    class inplace_stop_token;
    class inplace_stop_source;

    template <typename F>
    class inplace_stop_callback;

    class inplace_stop_callback_base
    {
    public:
        void execute() noexcept
        {
            execute_(this);
        }

    protected:
        using execute_fn = void(inplace_stop_callback_base* cb) noexcept;

        explicit inplace_stop_callback_base(inplace_stop_source* source, execute_fn* execute) noexcept : source(source), execute_(execute)
        {
        }

        void register_callback() noexcept;

        friend inplace_stop_source;

        inplace_stop_source* source;
        execute_fn* execute_;

        inplace_stop_callback_base* next = nullptr;
        inplace_stop_callback_base** prev_ptr = nullptr;

        bool* removed_during_callback = nullptr;
        std::atomic<bool> callback_completed{false};
    };

    class inplace_stop_source
    {
    public:
        inplace_stop_source() noexcept = default;

        inplace_stop_source(inplace_stop_source&&) = delete;
        inplace_stop_source(const inplace_stop_source&) = delete;

        inplace_stop_source& operator=(inplace_stop_source&&) = delete;
        inplace_stop_source& operator=(const inplace_stop_source&) = delete;

        bool request_stop() noexcept
        {
            if (!lock_unless_stop_requested(true))
                return true;

            notifying_thread_id = std::this_thread::get_id();

            while (callbacks != nullptr)
            {
                auto* callback = callbacks;

                callback->prev_ptr = nullptr;
                callbacks = callback->next;

                if (callbacks != nullptr)
                    callbacks->prev_ptr = &callbacks;

                state.store(stop_requested_flag, std::memory_order_release);

                bool removed_during_callback = false;
                callback->removed_during_callback = &removed_during_callback;

                callback->execute();

                if (!removed_during_callback)
                {
                    callback->removed_during_callback = nullptr;
                    callback->callback_completed.store(true, std::memory_order_release);
                }

                lock();
            }

            state.store(stop_requested_flag, std::memory_order_release);

            return false;
        }

        inplace_stop_token get_token() noexcept;

        bool stop_requested() const noexcept
        {
            return (state.load(std::memory_order_acquire) & stop_requested_flag) != 0;
        }

        ~inplace_stop_source()
        {
            assert((state.load(std::memory_order_relaxed) & locked_flag) == 0);
            assert(callbacks == nullptr);
        }

    private:
        friend inplace_stop_token;
        friend inplace_stop_callback_base;

        template <typename F>
        friend class inplace_stop_callback;

        std::uint8_t lock() noexcept
        {
            spin_wait spin;
            auto old_state = state.load(std::memory_order_relaxed);

            do
            {
                while ((old_state & locked_flag) != 0)
                {
                    spin.wait();
                    old_state = state.load(std::memory_order_relaxed);
                }
            }
            while (!state.compare_exchange_weak(old_state, old_state | locked_flag, std::memory_order_acquire, std::memory_order_relaxed));

            return old_state;
        }

        void unlock(std::uint8_t old_state) noexcept
        {
            state.store(old_state, std::memory_order_release);
        }

        bool lock_unless_stop_requested(bool requested) noexcept
        {
            spin_wait spin;
            auto old_state = state.load(std::memory_order_relaxed);

            do
            {
                while (true)
                {
                    if ((old_state & stop_requested_flag) != 0)
                        return false;
                    else if (old_state == 0)
                        break;
                    else
                    {
                        spin.wait();
                        old_state = state.load(std::memory_order_relaxed);
                    }
                }
            }
            while (!state.compare_exchange_weak(old_state, requested ? (locked_flag | stop_requested_flag) : locked_flag, std::memory_order_acq_rel, std::memory_order_relaxed));

            return true;
        }

        bool add_callback(inplace_stop_callback_base* callback) noexcept
        {
            if (!lock_unless_stop_requested(false))
                return false;

            callback->next = callbacks;
            callback->prev_ptr = &callbacks;

            if (callbacks != nullptr)
                callbacks->prev_ptr = &callback->next;

            callbacks = callback;
            unlock(0);

            return true;
        }

        void remove_callback(inplace_stop_callback_base* callback) noexcept
        {
            auto old_state = lock();

            if (callback->prev_ptr != nullptr)
            {
                *callback->prev_ptr = callback->next;

                if (callback->next != nullptr)
                    callback->next->prev_ptr = callback->prev_ptr;

                unlock(old_state);
            }
            else
            {
                auto id = notifying_thread_id;
                unlock(old_state);

                if (std::this_thread::get_id() == id)
                {
                    if (callback->removed_during_callback != nullptr) 
                        *callback->removed_during_callback = true;
                }
                else
                {
                    spin_wait spin;

                    while (!callback->callback_completed.load(std::memory_order_acquire))
                           spin.wait();
                }
            }
        }

        static constexpr std::uint8_t locked_flag = 2;
        static constexpr std::uint8_t stop_requested_flag = 1;

        std::atomic<std::uint8_t> state{0};
        inplace_stop_callback_base* callbacks = nullptr;

        std::thread::id notifying_thread_id;
    };

    class inplace_stop_token
    {
    public:
        template <typename F>
        using callback_type = inplace_stop_callback<F>;

        inplace_stop_token() noexcept
        {
        }

        inplace_stop_token(const inplace_stop_token& other) noexcept = default;
        inplace_stop_token& operator=(const inplace_stop_token& other) noexcept = default;

        bool stop_requested() const noexcept
        {
            return source != nullptr && source->stop_requested();
        }

        bool stop_possible() const noexcept
        {
            return source != nullptr;
        }

        void swap(inplace_stop_token& other) noexcept
        {
            std::swap(source, other.source);
        }

        friend bool operator==(const inplace_stop_token& a, const inplace_stop_token& b) noexcept
        {
            return a.source == b.source;
        }

        friend bool operator!=(const inplace_stop_token& a, const inplace_stop_token& b) noexcept
        {
            return !(a == b);
        }

    private:
        friend inplace_stop_source;

        template <typename F>
        friend class inplace_stop_callback;

        explicit inplace_stop_token(inplace_stop_source* source) noexcept : source(source)
        {
        }

        inplace_stop_source* source = nullptr;
    };

    inline inplace_stop_token inplace_stop_source::get_token() noexcept
    {
        return inplace_stop_token{this};
    }

    template <typename F>
    class inplace_stop_callback final : private inplace_stop_callback_base
    {
    public:
        template <typename T = F>
        requires std::convertible_to<T, F>
        explicit inplace_stop_callback(inplace_stop_token token, T&& func) noexcept(std::is_nothrow_constructible_v<F, T>) :
        inplace_stop_callback_base(token.source, &inplace_stop_callback::call), f(std::forward<T>(func))
        {
            this->register_callback();
        }

        ~inplace_stop_callback()
        {
            if (source != nullptr)
                source->remove_callback(this);
        }

    private:
        static void call(inplace_stop_callback_base* cb) noexcept
        {
            auto& self = *static_cast<inplace_stop_callback*>(cb);
            self.f();
        }

        [[no_unique_address]] F f;
    };

    inline void inplace_stop_callback_base::register_callback() noexcept
    {
        if (source != nullptr && !source->add_callback(this))
        {
            source = nullptr;
            execute();
        }
    }
}

#endif
