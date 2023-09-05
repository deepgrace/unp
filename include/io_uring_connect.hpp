//
// Copyright (c) 2023-present DeepGrace (complex dot invoke at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/deepgrace/unp
//

#ifndef IO_URING_CONNECT_HPP
#define IO_URING_CONNECT_HPP

namespace unp
{
    class connector : private completion_base
    {
    public:
        void start() noexcept
        {
            if (!context.is_running_on_io_thread())
            {
                execute_ = &connector::on_schedule_complete;
                context.schedule_remote(this);
            }
            else
                start_io();
        }

        static void on_schedule_complete(operation_base* op) noexcept
        {
            static_cast<connector*>(op)->start_io();
        }

        void start_io() noexcept
        {
            assert(context.is_running_on_io_thread());

            auto fill = [this](io_uring_sqe& sqe) noexcept
            {
                sqe.opcode = IORING_OP_CONNECT;
                sqe.fd = fd_;

                sqe.off = endpoint.size();
                sqe.addr = reinterpret_cast<std::uintptr_t>((void*)(endpoint.data()));

                sqe.user_data = reinterpret_cast<std::uintptr_t>(static_cast<completion_base*>(this));
                execute_ = &connector::on_connect;
            };

            if (!context.submit_io(fill))
            {
                execute_ = &connector::on_schedule_complete;
                context.schedule_pending_io(this);
            }
        }

        static void on_connect(operation_base* op) noexcept
        {
            auto& self = *static_cast<connector*>(op);

            if (self.result >= 0)
                self.receiver(std::error_code(), self.fd_);
            else
                self.receiver(std::error_code(-self.result, std::system_category()), self.fd_);
        }

        explicit connector(io_uring_context& context) noexcept : context(context)
        {
        }

        template <typename F>
        void async_connect(const unp::ip::tcp::endpoint& ep, F&& f)
        {
            endpoint = ep;

            async_socket(context, endpoint.protocol().family(), SOCK_STREAM, IPPROTO_TCP,
            [this, f](std::error_code ec, int fd)
            {
                if (!ec)
                {
                    open_socket(fd);
                    do_connect(f);
                }
            });
        }

        template <typename F>
        void do_connect(F&& f)
        {
            receiver = f;
            start();
        }

        void open_socket(int fd) noexcept
        {
            fd_ = fd;

            std::int32_t ret = 0;
            std::int32_t val = 1;

            ret = setsockopt(fd_, SOL_SOCKET, SO_REUSEPORT, &val, sizeof(val));
            assert(ret != -1);

            ret = setsockopt(fd_, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));
            assert(ret != -1);
        }

        int fd_ = -1;
        io_uring_context& context;

        unp::ip::tcp::endpoint endpoint;
        std::function<void(std::error_code, int)> receiver;
    };

    inline constexpr connect_impl<connector> async_connect {};
}

#endif
