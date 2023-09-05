//
// Copyright (c) 2023-present DeepGrace (complex dot invoke at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/deepgrace/unp
//

#ifndef ENDPOINT_HPP
#define ENDPOINT_HPP

#include <sstream>
#include <address.hpp>

namespace unp::ip
{
    class endpoint
    {
    public:
        endpoint() noexcept : data_()
        {
            data_.v4.sin_family = AF_INET;
            data_.v4.sin_port = 0;

            data_.v4.sin_addr.s_addr = INADDR_ANY;
        }

        endpoint(int family, unsigned short port_num) noexcept : data_()
        {
            if (family == AF_INET)
            {
                data_.v4.sin_family = AF_INET;
                data_.v4.sin_port = htons(port_num);

                data_.v4.sin_addr.s_addr = INADDR_ANY;
            }
            else
            {
                data_.v6.sin6_family = AF_INET6;
                data_.v6.sin6_port = htons(port_num);

                data_.v6.sin6_flowinfo = 0;
                data_.v6.sin6_scope_id = 0;

                for (int i = 0; i < 16; ++i)
                     data_.v6.sin6_addr.s6_addr[i] = 0;
            }
        }

        endpoint(const unp::ip::address& addr, unsigned short port_num) noexcept : data_()
        {
            if (addr.is_v4())
            {
                data_.v4.sin_family = AF_INET;
                data_.v4.sin_port = htons(port_num);

                data_.v4.sin_addr.s_addr = htonl(addr.to_v4().to_uint());
            }
            else
            {
                data_.v6.sin6_family = AF_INET6;
                data_.v6.sin6_port = htons(port_num);

                data_.v6.sin6_flowinfo = 0;

                unp::ip::address_v6 v6_addr = addr.to_v6();
                unp::ip::address_v6::bytes_type bytes = v6_addr.to_bytes();

                std::memcpy(data_.v6.sin6_addr.s6_addr, bytes.data(), 16);
                data_.v6.sin6_scope_id = static_cast<uint32_t>(v6_addr.scope_id());
            }
        }

        endpoint(const endpoint& other) noexcept : data_(other.data_)
        {
        }

        endpoint& operator=(const endpoint& other) noexcept
        {
            data_ = other.data_;

            return *this;
        }

        sockaddr* data() noexcept
        {
            return &data_.base;
        }

        const sockaddr* data() const noexcept
        {
            return &data_.base;
        }

        std::size_t size() const noexcept
        {
            if (is_v4())
                return sizeof(sockaddr_in);
            else
                return sizeof(sockaddr_in6);
        }

        void resize(std::size_t new_size)
        {
            if (new_size > sizeof(sockaddr_storage))
                throw(std::invalid_argument("out of size"));
        }

        std::size_t capacity() const noexcept
        {
            return sizeof(data_);
        }

        unsigned short port() const noexcept
        {
            if (is_v4())
                return ntohs(data_.v4.sin_port);
            else
                return ntohs(data_.v6.sin6_port);
        }

        void port(unsigned short port_num) noexcept
        {
            if (is_v4())
                data_.v4.sin_port = htons(port_num);
            else
                data_.v6.sin6_port = htons(port_num);
        }

        unp::ip::address address() const noexcept
        {
            if (is_v4())
                return unp::ip::address_v4(ntohl(data_.v4.sin_addr.s_addr));
            else
            {
                unp::ip::address_v6::bytes_type bytes;
                std::memcpy(bytes.data(), data_.v6.sin6_addr.s6_addr, 16);

                return unp::ip::address_v6(bytes, data_.v6.sin6_scope_id);
            }
        }

        void address(const unp::ip::address& addr) noexcept
        {
            endpoint ep(addr, port());
            data_ = ep.data_;
        }

        friend bool operator==(const endpoint& e1, const endpoint& e2) noexcept
        {
            return e1.address() == e2.address() && e1.port() == e2.port();
        }

        friend bool operator<(const endpoint& e1, const endpoint& e2) noexcept
        {
            if (e1.address() < e2.address())
                return true;

            if (e1.address() != e2.address())
                return false;

            return e1.port() < e2.port();
        }

        bool is_v4() const noexcept
        {
            return data_.base.sa_family == AF_INET;
        }

        std::string to_string() const
        {
            std::ostringstream os;
            os.imbue(std::locale::classic());

            if (is_v4())
                os << address();
            else
                os << '[' << address() << ']';

            os << ':' << port();

            return os.str();
        }

    private:
        union data_union
        {
            sockaddr base;

            sockaddr_in v4;
            sockaddr_in6 v6;
        };

        data_union data_;
    };
}

#endif
