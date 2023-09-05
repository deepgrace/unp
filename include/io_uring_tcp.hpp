//
// Copyright (c) 2023-present DeepGrace (complex dot invoke at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/deepgrace/unp
//

#ifndef IO_URING_TCP_HPP
#define IO_URING_TCP_HPP

#include <sys/socket.h>
#include <basic_endpoint.hpp>

namespace unp
{
    class acceptor;
}

namespace unp::ip
{
    class tcp
    {
    public:
        using acceptor = unp::acceptor;

        using socket = unp::async_file;
        using endpoint = basic_endpoint<tcp>;

        explicit tcp(int protocol_family) noexcept : family_(protocol_family)
        {
        }

        int type() const noexcept
        {
            return SOCK_STREAM;
        }

        int family() const noexcept
        {
            return family_;
        }

        int protocol() const noexcept
        {
            return IPPROTO_TCP;
        }

        static tcp v4() noexcept
        {
            return tcp(AF_INET);
        }

        static tcp v6() noexcept
        {
            return tcp(AF_INET6);
        }

        friend bool operator==(const tcp& p1, const tcp& p2)
        {
            return p1.family_ == p2.family_;
        }

        friend bool operator!=(const tcp& p1, const tcp& p2)
        {
            return p1.family_ != p2.family_;
        }

    private:
        int family_;
    };
}

#endif
