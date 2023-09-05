//
// Copyright (c) 2023-present DeepGrace (complex dot invoke at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/deepgrace/unp
//

#ifndef INET_OPS_HPP
#define INET_OPS_HPP

#include <cstring>
#include <net/if.h>

namespace inet_ops
{
    constexpr int max_addr_v4_str_len = INET_ADDRSTRLEN;
    constexpr int max_addr_v6_str_len = INET6_ADDRSTRLEN + 1 + IF_NAMESIZE;

    inline const char* inet_ntop(int af, const void* src, char* dest, size_t length, unsigned long scope_id = 0)
    {
        const char* result = ::inet_ntop(af, src, dest, static_cast<int>(length));

        if (result == 0)
            return result;

        if (af == AF_INET6 && scope_id != 0)
        {
            char if_name[(IF_NAMESIZE > 21 ? IF_NAMESIZE : 21) + 1] = "%";
            const in6_addr* ipv6_address = static_cast<const in6_addr*>(src);

            bool is_link_local = ((ipv6_address->s6_addr[0] == 0xfe) && ((ipv6_address->s6_addr[1] & 0xc0) == 0x80));
            bool is_multicast_link_local = ((ipv6_address->s6_addr[0] == 0xff) && ((ipv6_address->s6_addr[1] & 0x0f) == 0x02));

            if ((!is_link_local && !is_multicast_link_local) || if_indextoname(static_cast<unsigned>(scope_id), if_name + 1) == 0)
                std::snprintf(if_name + 1, sizeof(if_name) - 1, "%lu", scope_id);

            std::strcat(dest, if_name);
        }

        return result;
    }

    inline int inet_pton(int af, const char* src, void* dest, unsigned long* scope_id = 0)
    {
        const bool is_v6 = af == AF_INET6;
        const char* if_name = is_v6 ? std::strchr(src, '%') : 0;

        char src_buf[max_addr_v6_str_len + 1];
        const char* src_ptr = src;

        if (if_name != 0)
        {
            if (if_name - src > max_addr_v6_str_len)
                return 0;

            std::memcpy(src_buf, src, if_name - src);

            src_buf[if_name - src] = 0;
            src_ptr = src_buf;
        }

        int result = ::inet_pton(af, src_ptr, dest);

        if (result <= 0)
            return result;

        if (is_v6 && scope_id)
        {
            *scope_id = 0;

            if (if_name != 0)
            {
                in6_addr* ipv6_address = static_cast<in6_addr*>(dest);

                bool is_link_local = ((ipv6_address->s6_addr[0] == 0xfe) && ((ipv6_address->s6_addr[1] & 0xc0) == 0x80));
                bool is_multicast_link_local = ((ipv6_address->s6_addr[0] == 0xff) && ((ipv6_address->s6_addr[1] & 0x0f) == 0x02));

                if (is_link_local || is_multicast_link_local)
                    *scope_id = if_nametoindex(if_name + 1);

                if (*scope_id == 0)
                    *scope_id = std::atoi(if_name + 1);
            }
        }

        return result;
    }
}

#endif
