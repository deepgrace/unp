//
// Copyright (c) 2023-present DeepGrace (complex dot invoke at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/deepgrace/unp
//

#ifndef IO_URING_ACCEPT_HPP
#define IO_URING_ACCEPT_HPP

namespace unp
{
    class acceptor : private completion_base
    {
    public:
        using socket_t = async_file;

        void start() noexcept
        {
            if (!context.is_running_on_io_thread())
            {
                execute_ = &acceptor::on_schedule_complete;
                context.schedule_remote(this);
            }
            else
                start_io();
        }

        static void on_schedule_complete(operation_base* op) noexcept
        {
            static_cast<acceptor*>(op)->start_io();
        }

        void start_io() noexcept
        {
            assert(context.is_running_on_io_thread());

            auto fill = [this](io_uring_sqe& sqe) noexcept
            {
                sqe.opcode = IORING_OP_ACCEPT;
                sqe.accept_flags = SOCK_NONBLOCK;

                sqe.fd = fd_;
                sqe.user_data = reinterpret_cast<std::uintptr_t>(static_cast<completion_base*>(this));

                execute_ = &acceptor::on_accept;
            };

            if (!context.submit_io(fill))
            {
                execute_ = &acceptor::on_schedule_complete;
                context.schedule_pending_io(this);
            }
        }

        static void on_accept(operation_base* op) noexcept
        {
            auto& self = *static_cast<acceptor*>(op);

            if (self.result >= 0)
                self.receiver(std::error_code(), socket_t(self.context, self.result));
            else
                self.receiver(std::error_code(-self.result, std::system_category()), socket_t(self.context));
        }

        explicit acceptor(io_uring_context& context, const unp::ip::tcp::endpoint& endpoint) noexcept : context(context), endpoint(endpoint)
        {
        }

        template <typename F>
        void async_accept(F&& f)
        {
            if (fd_ < 0)
                async_socket(context, endpoint.protocol().family(), SOCK_STREAM | SOCK_CLOEXEC | SOCK_NONBLOCK, IPPROTO_TCP,
                [this, f](std::error_code ec, int fd)
                {
                    if (!ec)
                    {
                        open_socket(fd);
                        do_accept(f);
                    }
                });
            else
                do_accept(f);
        }

        template <typename F>
        void do_accept(F&& f)
        {
            receiver = f;
            start();
        }

        void close()
        {
            ::close(fd_);
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

            ret = bind(fd_, endpoint.data(), endpoint.size());
            assert(ret != -1);

            ret = listen(fd_, 4096);
            assert(ret != -1);
        }

    private:
        io_uring_context& context;
        unp::ip::tcp::endpoint endpoint;

        int fd_ = -1;
        std::function<void(std::error_code, socket_t)> receiver;
    };
}

#endif
