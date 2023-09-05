//
// Copyright (c) 2023-present DeepGrace (complex dot invoke at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/deepgrace/unp
//

#ifndef SCOPE_GUARD_HPP
#define SCOPE_GUARD_HPP

#include <utility>
#include <type_traits>

namespace unp
{
    template <typename F>
    class scope_guard
    {
    public:
        scope_guard(F&& f) noexcept : f(std::forward<F>(f))
        {
        }

        void release() noexcept
        {
            released_ = true;
        }

        void reset() noexcept
        {
            static_assert(noexcept(std::forward<F>(f)()));

            if (!std::exchange(released_, true))
                std::forward<F>(f)();
        }

        ~scope_guard()
        {
            reset();
        }

    private:
        static_assert(std::is_nothrow_move_constructible_v<F>);

        [[no_unique_address]] F f;
        bool released_ = false;
    };

    template <typename F>
    scope_guard(F&& f) -> scope_guard<F>;
}

#endif
