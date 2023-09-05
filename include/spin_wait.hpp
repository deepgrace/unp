//
// Copyright (c) 2023-present DeepGrace (complex dot invoke at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/deepgrace/unp
//

#ifndef SPIN_WAIT_HPP
#define SPIN_WAIT_HPP

#include <thread>

namespace unp
{
    class spin_wait
    {
    public:
        spin_wait() noexcept = default;

        void wait() noexcept
        {
            if (count++ < yield_threshold)
            {
            }
            else
            {
                if (count == 0)
                    count = yield_threshold;

                std::this_thread::yield();
            }
        }

    private:
        std::uint32_t count = 0;
        static constexpr std::uint32_t yield_threshold = 20;
    };
}

#endif
