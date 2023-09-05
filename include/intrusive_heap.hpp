//
// Copyright (c) 2023-present DeepGrace (complex dot invoke at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/deepgrace/unp
//

#ifndef INTRUSIVE_HEAP_HPP
#define INTRUSIVE_HEAP_HPP

namespace unp
{
    template <typename T, T* T::*NEXT, T* T::*PREV, typename U, U T::*KEY>
    class intrusive_heap
    {
    public:
        intrusive_heap() noexcept : head(nullptr)
        {
        }

        bool empty() const noexcept
        {
            return head == nullptr;
        }

        T* top() const noexcept
        {
            assert(!empty());

            return head;
        }

        T* pop() noexcept
        {
            assert(!empty());
            T* item = head;

            head = item->*NEXT;

            if (head != nullptr)
                head->*PREV = nullptr;

            return item;
        }

        void insert(T* item) noexcept
        {
            if (head == nullptr)
            {
                head = item;

                item->*NEXT = nullptr;
                item->*PREV = nullptr;
            }
            else if (item->*KEY < head->*KEY)
            {
                item->*NEXT = head;
                item->*PREV = nullptr;

                head->*PREV = item;
                head = item;
            }
            else
            {
                auto* insert_next = head;

                while (insert_next->*NEXT != nullptr && insert_next->*NEXT->*KEY <= item->*KEY)
                       insert_next = insert_next->*NEXT;

                auto* insert_prev = insert_next->*NEXT;

                item->*PREV = insert_next;
                item->*NEXT = insert_prev;

                insert_next->*NEXT = item;

                if (insert_prev != nullptr)
                    insert_prev->*PREV = item;
            }
        }

        void remove(T* item) noexcept
        {
            auto* prev = item->*PREV;
            auto* next = item->*NEXT;

            if (prev != nullptr)
                prev->*NEXT = next;
            else
            {
                assert(head == item);
                head = next;
            }

            if (next != nullptr)
                next->*PREV = prev;
        }

        ~intrusive_heap()
        {
            T* item = head;

            if (item != nullptr)
                assert(item->*PREV == nullptr);

            while (item != nullptr)
            {
                if (item->*NEXT != nullptr)
                    assert(item->*NEXT->*PREV == item);

                item = item->*NEXT;
            }

            assert(empty());
        }

    private:
        T* head;
    };
}

#endif
