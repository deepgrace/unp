//
// Copyright (c) 2023-present DeepGrace (complex dot invoke at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/deepgrace/unp
//

#ifndef IO_URING_POST_HPP
#define IO_URING_POST_HPP

namespace unp
{
    class post_operation : private operation_base
    {
    public:
        void start() noexcept
        {
            context.schedule_impl(this);
        }

        static void execute_impl(operation_base* op) noexcept
        {
            auto& self = *static_cast<post_operation*>(op);
            self.receiver();
        }

        explicit post_operation(io_uring_context& context) noexcept : context(context)
        {
            execute_ = &execute_impl;
        }

        template <typename F>
        void async_post(F&& f)
        {
            receiver = f;
            start();
        }

        io_uring_context& context;
        std::function<void()> receiver; 
    };
}

#endif
