//
// Copyright (c) 2023-present DeepGrace (complex dot invoke at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/deepgrace/unp
//

#ifndef INTRUSIVE_STACK_HPP
#define INTRUSIVE_STACK_HPP

#include <utility>

namespace unp
{
    template <typename T, T* T::*NEXT>
    class intrusive_stack
    {
    public:
        intrusive_stack() : head(nullptr)
        {
        }

        intrusive_stack(const intrusive_stack&) = delete;

        intrusive_stack(intrusive_stack&& other) noexcept : head(std::exchange(other.head, nullptr))
        {
        }

        intrusive_stack& operator=(const intrusive_stack&) = delete;
        intrusive_stack& operator=(intrusive_stack&&) = delete;

        static intrusive_stack adopt(T* head) noexcept
        {
            intrusive_stack stack;
            stack.head = head;

            return stack;
        }

        T* release() noexcept
        {
            return std::exchange(head, nullptr);
        }

        [[nodiscard]] bool empty() const noexcept
        {
            return head == nullptr;
        }

        void push_front(T* item) noexcept
        {
            item->*NEXT = head;
            head = item;
        }

        [[nodiscard]] T* pop_front() noexcept
        {
            assert(!empty());

            T* item = head;
            head = item->*NEXT;

            return item;
        }

        ~intrusive_stack()
        {
            assert(empty());
        }

    private:
        T* head;
    };
}

#endif
