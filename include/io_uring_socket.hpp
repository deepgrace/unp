//
// Copyright (c) 2023-present DeepGrace (complex dot invoke at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/deepgrace/unp
//

#ifndef IO_URING_SOCKET_HPP
#define IO_URING_SOCKET_HPP

namespace unp
{
    class socket_operation : private completion_base
    {
    public:
        void start() noexcept
        {
            if (!context.is_running_on_io_thread())
            {
                execute_ = &socket_operation::on_schedule_complete;
                context.schedule_remote(this);
            }
            else
                start_io();
        }

        static void on_schedule_complete(operation_base* op) noexcept
        {
            static_cast<socket_operation*>(op)->start_io();
        }

        void start_io() noexcept
        {
            assert(context.is_running_on_io_thread());

            auto fill = [this](io_uring_sqe& sqe) noexcept
            {
                sqe.opcode = IORING_OP_SOCKET;
                sqe.rw_flags = 0;

                sqe.fd = domain;
                sqe.off = type;

                sqe.len = protocol;
                sqe.user_data = reinterpret_cast<std::uintptr_t>(static_cast<completion_base*>(this));

                execute_ = &socket_operation::on_socket;
            };

            if (!context.submit_io(fill))
            {
                execute_ = &socket_operation::on_schedule_complete;
                context.schedule_pending_io(this);
            }
        }

        static void on_socket(operation_base* op) noexcept
        {
            auto& self = *static_cast<socket_operation*>(op);

            if (self.result >= 0)
                self.receiver(std::error_code(), self.result);
            else
                self.receiver(std::error_code(-self.result, std::system_category()), self.result);
        }

        explicit socket_operation(io_uring_context& context, int domain, int type, int protocol) noexcept : context(context), domain(domain), type(type), protocol(protocol)
        {
        }

        template <typename F>
        void async_socket(F&& f)
        {
            receiver = f;
            start();
        }

        io_uring_context& context;

        int domain;
        int type;

        int protocol;
        std::function<void(std::error_code, int)> receiver;
    };
}

#endif
