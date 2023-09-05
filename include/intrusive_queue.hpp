//
// Copyright (c) 2023-present DeepGrace (complex dot invoke at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/deepgrace/unp
//

#ifndef INTRUSIVE_QUEUE_HPP
#define INTRUSIVE_QUEUE_HPP

#include <tuple>
#include <utility>

namespace unp
{
    template <typename T, T* T::*NEXT>
    class intrusive_queue
    {
    public:
        intrusive_queue() noexcept = default;

        intrusive_queue(intrusive_queue&& other) noexcept : head(std::exchange(other.head, nullptr)), tail(std::exchange(other.tail, nullptr))
        {
        }

        intrusive_queue& operator=(intrusive_queue other) noexcept
        {
            std::swap(head, other.head);
            std::swap(tail, other.tail);

            return *this;
        }

        static intrusive_queue make_reversed(T* list) noexcept
        {
            T* new_head = nullptr;
            T* new_tail = list;

            while (list != nullptr)
            {
                T* next = list->*NEXT;
                list->*NEXT = new_head;

                new_head = list;
                list = next;
            }

            intrusive_queue result;

            result.head = new_head;
            result.tail = new_tail;

            return result;
        }

        [[nodiscard]] bool empty() const noexcept
        {
            return head == nullptr;
        }

        [[nodiscard]] T* pop_front() noexcept
        {
            assert(!empty());
            T* item = std::exchange(head, head->*NEXT);

            if (head == nullptr)
                tail = nullptr;

            return item;
        }

        void push_front(T* item) noexcept
        {
            assert(item != nullptr);

            item->*NEXT = head;
            head = item;

            if (tail == nullptr)
                tail = item;
        }

        void push_back(T* item) noexcept
        {
            assert(item != nullptr);
            item->*NEXT = nullptr;

            if (tail == nullptr)
                head = item;
            else
                tail->*NEXT = item;

            tail = item;
        }

        void append(intrusive_queue other) noexcept
        {
            if (other.empty())
                return;

            auto* other_head = std::exchange(other.head, nullptr);

            if (empty())
                head = other_head;
            else
                tail->*NEXT = other_head;

            tail = std::exchange(other.tail, nullptr);
        }

        void prepend(intrusive_queue other) noexcept
        {
            if (other.empty())
                return;

            other.tail->*NEXT = head;
            head = other.head;

            if (tail == nullptr)
                tail = other.tail;

            other.tail = nullptr;
            other.head = nullptr;
        }

        ~intrusive_queue()
        {
            assert(empty());
        }

    private:
        T* head = nullptr;
        T* tail = nullptr;
    };
}

#endif
