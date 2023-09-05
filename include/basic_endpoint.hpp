//
// Copyright (c) 2023-present DeepGrace (complex dot invoke at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/deepgrace/unp
//

#ifndef BASIC_ENDPOINT_HPP
#define BASIC_ENDPOINT_HPP

#include <endpoint.hpp>

namespace unp::ip
{
    using port_type = uint_least16_t;

    template <typename T>
    class basic_endpoint
    {
    public:
        using protocol_type = T;
        using data_type = sockaddr;

        basic_endpoint() noexcept : impl()
        {
        }

        basic_endpoint(const T& t, port_type port_num) noexcept : impl(t.family(), port_num)
        {
        }

        basic_endpoint(const unp::ip::address& addr, port_type port_num) noexcept : impl(addr, port_num)
        {
        }

        basic_endpoint(const basic_endpoint& other) noexcept : impl(other.impl)
        {
        }

        basic_endpoint(basic_endpoint&& other) noexcept : impl(other.impl)
        {
        }

        basic_endpoint& operator=(const basic_endpoint& other) noexcept
        {
            impl = other.impl;

            return *this;
        }

        basic_endpoint& operator=(basic_endpoint&& other) noexcept
        {
            impl = other.impl;

            return *this;
        }

        bool is_v4() const noexcept
        {
            return impl.is_v4();
        }

        protocol_type protocol() const noexcept
        {
            if (is_v4())
                return T::v4();

            return T::v6();
        }

        data_type* data() noexcept
        {
            return impl.data();
        }

        const data_type* data() const noexcept
        {
            return impl.data();
        }

        std::size_t size() const noexcept
        {
            return impl.size();
        }

        void resize(std::size_t new_size)
        {
            impl.resize(new_size);
        }

        std::size_t capacity() const noexcept
        {
            return impl.capacity();
        }

        port_type port() const noexcept
        {
            return impl.port();
        }

        void port(port_type port_num) noexcept
        {
            impl.port(port_num);
        }

        unp::ip::address address() const noexcept
        {
            return impl.address();
        }

        void address(const unp::ip::address& addr) noexcept
        {
            impl.address(addr);
        }

        friend bool operator==(const basic_endpoint<T>& e1, const basic_endpoint<T>& e2) noexcept
        {
            return e1.impl == e2.impl;
        }

        friend bool operator!=(const basic_endpoint<T>& e1, const basic_endpoint<T>& e2) noexcept
        {
            return !(e1 == e2);
        }

        friend bool operator<(const basic_endpoint<T>& e1, const basic_endpoint<T>& e2) noexcept
        {
            return e1.impl < e2.impl;
        }

        friend bool operator>(const basic_endpoint<T>& e1, const basic_endpoint<T>& e2) noexcept
        {
            return e2.impl < e1.impl;
        }

        friend bool operator<=(const basic_endpoint<T>& e1, const basic_endpoint<T>& e2) noexcept
        {
            return !(e2 < e1);
        }

        friend bool operator>=(const basic_endpoint<T>& e1, const basic_endpoint<T>& e2) noexcept
        {
            return !(e1 < e2);
        }

    private:
        endpoint impl;
    };

    template <typename S, typename T>
    constexpr decltype(auto) operator<<(S& s, const basic_endpoint<T>& endpoint)
    {
        unp::ip::endpoint ep(endpoint.address(), endpoint.port());

        return s << ep.to_string();
    }
}

namespace std
{
    template <typename T>
    struct hash<unp::ip::basic_endpoint<T>>
    {
        std::size_t operator()(const unp::ip::basic_endpoint<T>& ep) const noexcept
        {
            std::size_t haddr = std::hash<unp::ip::address>()(ep.address());
            std::size_t hport = std::hash<unsigned short>()(ep.port());

            return haddr ^ (hport + 0x9e3779b9 + (haddr << 6) + (haddr >> 2));
        }
    };
}

#endif
