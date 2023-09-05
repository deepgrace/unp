//
// Copyright (c) 2023-present DeepGrace (complex dot invoke at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/deepgrace/unp
//

#ifndef IO_URING_READ_WRITE_HPP
#define IO_URING_READ_WRITE_HPP

namespace unp
{
    template <typename T, io_uring_op opcode, bool B>
    class rw_type : private completion_base
    {
    public:
        using buffer_t = std::span<T>;

        static constexpr bool is_tcp = opcode == IORING_OP_READV || opcode == IORING_OP_WRITEV;
        static constexpr bool is_udp = opcode == IORING_OP_RECVMSG || opcode == IORING_OP_SENDMSG;

        using seekable_t = std::conditional_t<is_udp, unp::ip::udp::endpoint&, bool>;

        void start() noexcept
        {
            if (!context.is_running_on_io_thread())
            {
                execute_ = &rw_type::on_schedule_complete;
                context.schedule_remote(this);
            }
            else
                start_io();
        }

        static void on_schedule_complete(operation_base* op) noexcept
        {
            static_cast<rw_type*>(op)->start_io();
        }

        void start_io() noexcept
        {
            assert(context.is_running_on_io_thread());

            auto fill = [this](io_uring_sqe& sqe) noexcept
            {
                sqe.opcode = opcode;
                sqe.fd = fd;

                if constexpr(is_udp)
                {
                    sqe.addr = reinterpret_cast<std::uintptr_t>(&msg);
                    sqe.msg_flags = 0;
                }
                else
                {
                    sqe.addr = reinterpret_cast<std::uintptr_t>(&buff);
                    sqe.len = 1;
                }

                sqe.off = offset;
                sqe.user_data = reinterpret_cast<std::uintptr_t>(static_cast<completion_base*>(this));

                execute_ = &rw_type::on_io_complete;
            };

            if (!context.submit_io(fill))
            {
                execute_ = &rw_type::on_schedule_complete;
                context.schedule_pending_io(this);
            }
        }

        static void on_io_complete(operation_base* op) noexcept
        {
            auto& self = *static_cast<rw_type*>(op);

            if (self.result >= 0)
            {
                self.bytes += self.result;

                if constexpr(is_tcp)
                    self.offset += self.seekable * self.result;

                if ((B && (!self.bytes || self.bytes == self.size)) || !B)
                {
                    if (!self.bytes)
                        self.receiver(std::make_error_code(std::errc::no_message), self.bytes);
                    else
                        self.receiver(std::error_code(), self.bytes);
                }
                else
                {
                    self.buffer_ = unp::advance(self.buffer_, self.bytes);

                    self.init();
                    self.start();
                }
            }
            else
                self.receiver(std::error_code(-self.result, std::system_category()), self.result);
        }

    public:
        explicit rw_type(io_uring_context& context, int fd, offset_t offset, buffer_t buffer, seekable_t seekable) noexcept :
        context(context), fd(fd), offset(offset), buffer_(buffer), seekable(seekable), size(buffer.size())
        {
            init();
        }

        template <typename F>
        void async_io(F&& f)
        {
            receiver = f;
            start();
        }

        void init()
        {
            buff.iov_base = (void*)buffer_.data();
            buff.iov_len = buffer_.size();

            if constexpr(is_udp)
            {
                msg.msg_name = seekable.data();
                msg.msg_namelen = seekable.size();

                msg.msg_iov = &buff;
                msg.msg_iovlen = 1;
            }
        }

        io_uring_context& context;

        int fd;
        offset_t offset;

        buffer_t buffer_;
        seekable_t seekable;

        size_t size;
        size_t bytes = 0;

        iovec buff;
        msghdr msg;

        std::function<void(std::error_code, int)> receiver;
    };

    using read_type = rw_type<std::byte, IORING_OP_READV, 1>;
    using write_type = rw_type<const std::byte, IORING_OP_WRITEV, 1>;

    using read_some_type = rw_type<std::byte, IORING_OP_READV, 0>;
    using write_some_type = rw_type<const std::byte, IORING_OP_WRITEV, 0>;

    using receive_from_type = rw_type<std::byte, IORING_OP_RECVMSG, 0>;
    using send_to_type = rw_type<const std::byte, IORING_OP_SENDMSG, 0>;
}

#endif
