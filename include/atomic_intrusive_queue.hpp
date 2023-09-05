//
// Copyright (c) 2023-present DeepGrace (complex dot invoke at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/deepgrace/unp
//

#ifndef ATOMIC_INTRUSIVE_QUEUE_HPP
#define ATOMIC_INTRUSIVE_QUEUE_HPP

#include <atomic>
#include <utility>
#include <intrusive_queue.hpp>
#include <intrusive_stack.hpp>

namespace unp
{
    template <typename T, T* T::*NEXT>
    class atomic_intrusive_queue
    {
    public:
        atomic_intrusive_queue() noexcept : head(nullptr)
        {
        }

        explicit atomic_intrusive_queue(bool active) noexcept : head(active ? nullptr : produce_inactive_value())
        {
        }

        atomic_intrusive_queue(const atomic_intrusive_queue&) = delete;
        atomic_intrusive_queue(atomic_intrusive_queue&&) = delete;

        atomic_intrusive_queue& operator=(const atomic_intrusive_queue&) = delete;
        atomic_intrusive_queue& operator=(atomic_intrusive_queue&&) = delete;

        [[nodiscard]] bool mark_active() noexcept
        {
            void* old_value = produce_inactive_value();

            return head.compare_exchange_strong(old_value, nullptr, std::memory_order_acquire, std::memory_order_relaxed);
        }

        [[nodiscard]] bool enqueue_or_mark_active(T* item) noexcept
        {
            void* new_value;

            void* const inactive = produce_inactive_value();
            void* old_value = head.load(std::memory_order_relaxed);

            do
            {
                if (old_value == inactive)
                    new_value = nullptr;
                else
                {
                    item->*NEXT = static_cast<T*>(old_value);
                    new_value = item;
                }
            }
            while (!head.compare_exchange_weak(old_value, new_value, std::memory_order_acq_rel));

            return old_value != inactive;
        }

        [[nodiscard]] bool enqueue(T* item) noexcept
        {
            void* const inactive = produce_inactive_value();
            void* old_value = head.load(std::memory_order_relaxed);

            do
            {
                item->*NEXT = (old_value == inactive) ? nullptr : static_cast<T*>(old_value);
            }
            while (!head.compare_exchange_weak(old_value, item, std::memory_order_acq_rel));

            return old_value == inactive;
        }

        [[nodiscard]] intrusive_queue<T, NEXT> dequeue_all() noexcept
        {
            void* value = head.load(std::memory_order_relaxed);

            if (value == nullptr)
                return {};

            assert(value != produce_inactive_value());
            value = head.exchange(nullptr, std::memory_order_acquire);

            assert(value != produce_inactive_value());
            assert(value != nullptr);

            return intrusive_queue<T, NEXT>::make_reversed(static_cast<T*>(value));
        }

        [[nodiscard]] intrusive_stack<T, NEXT> dequeue_all_reversed() noexcept
        {
            void* value = head.load(std::memory_order_relaxed);

            if (value == nullptr)
                return {};

            assert(value != produce_inactive_value());
            value = head.exchange(nullptr, std::memory_order_acquire);

            assert(value != produce_inactive_value());
            assert(value != nullptr);

            return intrusive_stack<T, NEXT>::adopt(static_cast<T*>(value));
        }

        [[nodiscard]] bool mark_inactive() noexcept
        {
            void* const inactive = produce_inactive_value();
            void* old_value = head.load(std::memory_order_relaxed);

            if (old_value == nullptr)
            {
                if (head.compare_exchange_strong(old_value, inactive, std::memory_order_release, std::memory_order_relaxed))
                    return true;
            }

            assert(old_value != nullptr);
            assert(old_value != inactive);

            return false;
        }

        [[nodiscard]] intrusive_queue<T, NEXT> mark_inactive_or_dequeue_all() noexcept
        {
            if (mark_inactive())
                return {};

            void* old_value = head.exchange(nullptr, std::memory_order_acquire);

            assert(old_value != nullptr);
            assert(old_value != produce_inactive_value());

            return intrusive_queue<T, NEXT>::make_reversed(static_cast<T*>(old_value));
        }

        ~atomic_intrusive_queue()
        {
            assert(head.load(std::memory_order_relaxed) == nullptr || head.load(std::memory_order_relaxed) == produce_inactive_value());
        }

    private:
        void* produce_inactive_value() const noexcept
        {
            return const_cast<void*>(static_cast<const void*>(&head));
        }

        std::atomic<void*> head;
    };
}

#endif
