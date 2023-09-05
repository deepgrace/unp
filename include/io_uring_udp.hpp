//
// Copyright (c) 2023-present DeepGrace (complex dot invoke at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/deepgrace/unp
//

#ifndef IO_URING_UDP_HPP
#define IO_URING_UDP_HPP

#include <sys/socket.h>
#include <basic_endpoint.hpp>

namespace unp
{
    class datagram_socket;
}

namespace unp::ip
{
    class udp
    {
    public:
        using socket = unp::datagram_socket;
        using endpoint = basic_endpoint<udp>;

        explicit udp(int protocol_family) noexcept : family_(protocol_family)
        {
        }

        int type() const noexcept
        {
            return SOCK_DGRAM;
        }

        int family() const noexcept
        {
            return family_;
        }

        int protocol() const noexcept
        {
            return IPPROTO_UDP;
        }

        static udp v4() noexcept
        {
            return udp(AF_INET);
        }

        static udp v6() noexcept
        {
            return udp(AF_INET6);
        }

        friend bool operator==(const udp& p1, const udp& p2)
        {
            return p1.family_ == p2.family_;
        }

        friend bool operator!=(const udp& p1, const udp& p2)
        {
            return p1.family_ != p2.family_;
        }

    private:
        int family_;
    };
}

#endif
