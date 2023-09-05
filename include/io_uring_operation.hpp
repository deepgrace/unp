//
// Copyright (c) 2023-present DeepGrace (complex dot invoke at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/deepgrace/unp
//

#ifndef IO_URING_OPERATION_HPP
#define IO_URING_OPERATION_HPP

namespace unp
{
    template <typename T, bool B>
    struct async_io
    {
        template <typename Stream, typename Buffer, typename F>
        requires B
        constexpr decltype(auto) operator()(Stream& stream, const Buffer& buffer, F&& f) const
        {
            offset_t offset = 0;
            constexpr auto seekable = requires { stream.offset; };

            if constexpr(seekable)
                offset = stream.offset;

            auto p = std::make_shared<T>(stream.get_context(), stream.get_fd(), offset, buffer, seekable);

            p->async_io([p, f, &stream](std::error_code ec, std::size_t bytes_transferred)
            {
                if (!ec)
                {
                    if constexpr(seekable)
                        stream.offset += bytes_transferred;
                }

                f(ec, bytes_transferred);
            });
        }

        template <typename Stream, typename Buffer, typename F>
        requires (!B)
        constexpr decltype(auto) operator()(Stream& stream, offset_t offset, const Buffer& buffer, F&& f) const
        {
            auto p = std::make_shared<T>(stream.get_context(), stream.get_fd(), offset, buffer, false);

            p->async_io([p, f](std::error_code ec, std::size_t bytes_transferred)
            {
                f(ec, bytes_transferred);
            });
        }

        template <typename Stream, typename Buffer, typename Endpoint, typename F>
        requires (!B)
        constexpr decltype(auto) operator()(Stream& stream, const Buffer& buffer, Endpoint& endpoint, F&& f) const
        {
            auto p = std::make_shared<T>(stream.get_context(), stream.get_fd(), 0, buffer, endpoint);

            p->async_io([p, f](std::error_code ec, std::size_t bytes_transferred)
            {
                f(ec, bytes_transferred);
            });
        }
    };

    template <typename T>
    struct post_impl
    {
        template <typename Context, typename F>
        constexpr decltype(auto) operator()(Context& context, F&& f) const
        {
            auto p = std::make_shared<T>(context);

            p->async_post([p, f]
            {
                f();
            });
        }
    };

    template <typename T>
    struct connect_impl
    {
        template <typename Stream, typename Endpoint, typename F>
        constexpr decltype(auto) operator()(Stream& stream, const Endpoint& endpoint, F&& f) const
        {
            auto p = std::make_shared<T>(stream.get_context());

            p->async_connect(endpoint, [&stream, p, f](std::error_code ec, int fd)
            {
                stream.reset(fd);
                f(ec, fd);
            });
        }
    };

    template <typename T>
    struct socket_impl
    {
        template <typename Context, typename F>
        constexpr decltype(auto) operator()(Context& context, int domain, int type, int protocol, F&& f) const
        {
            auto p = std::make_shared<T>(context, domain, type, protocol);

            p->async_socket([p, f](std::error_code ec, int fd)
            {
                f(ec, fd);
            });
        }
    };

    inline constexpr async_io<read_type, 1> async_read {};
    inline constexpr async_io<write_type, 1> async_write {};

    inline constexpr async_io<read_some_type, 1> async_read_some {};
    inline constexpr async_io<write_some_type, 1> async_write_some {};

    inline constexpr async_io<read_some_type, 0> async_read_some_at {};
    inline constexpr async_io<write_some_type, 0> async_write_some_at {};

    inline constexpr async_io<receive_from_type, 0> async_receive_from {};
    inline constexpr async_io<send_to_type, 0> async_send_to {};

    inline constexpr post_impl<post_operation> post {};
    inline constexpr socket_impl<socket_operation> async_socket {};
}

#endif
