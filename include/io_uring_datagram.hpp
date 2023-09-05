//
// Copyright (c) 2023-present DeepGrace (complex dot invoke at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/deepgrace/unp
//

#ifndef IO_URING_DATAGRAM_HPP
#define IO_URING_DATAGRAM_HPP

#include <sys/socket.h>

namespace unp
{
    class datagram_socket
    {
    public:
        enum shutdown_type
        {
            shutdown_receive = SHUT_RD,
            shutdown_send = SHUT_WR,

            shutdown_both = SHUT_RDWR
        };

        explicit datagram_socket(io_uring_context& context, const unp::ip::udp::endpoint& endpoint) noexcept :
        context(context)
        {
            int fd = socket(endpoint.protocol().family(), SOCK_DGRAM, IPPROTO_UDP);

            std::int32_t ret = 0;
            std::int32_t val = 1;

            ret = setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, &val, sizeof(val));
            assert(ret != -1);

            ret = setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));
            assert(ret != -1);

            ret = bind(fd, endpoint.data(), endpoint.size());
            assert(ret != -1);

            reset(fd);
        }

        template <typename T, typename Buffer, typename F>
        void async_io(const Buffer& buffer, unp::ip::udp::endpoint& endpoint, F&& f)
        {
            auto p = std::make_shared<T>(get_context(), get_fd(), 0, buffer, endpoint);

            p->async_io([p, f](std::error_code ec, std::size_t bytes_transferred)
            {
                f(ec, bytes_transferred);
            });
        }

        template <typename Buffer, typename F>
        void async_receive_from(const Buffer& buffer, unp::ip::udp::endpoint& endpoint, F&& f)
        {
            async_io<receive_from_type>(buffer, endpoint, std::forward<F>(f));      
        }

        template <typename Buffer, typename F>
        void async_send_to(const Buffer& buffer, unp::ip::udp::endpoint& endpoint, F&& f)
        {
            async_io<send_to_type>(buffer, endpoint, std::forward<F>(f));      
        }

        int get_fd()
        {
            return fd_.get();
        }

        io_uring_context& get_context()
        {
            return context;
        }

        bool is_open()
        {
            return fd_.valid();
        }

        void reset(int fd)
        {
            fd_.reset(fd);
        }

        void close()
        {
            fd_.close();
        }

        void shutdown(shutdown_type type)
        {
            ::shutdown(get_fd(), type);
        }

        io_uring_context& context;
        safe_file_descriptor fd_;
    };
}

#endif
