//
// Copyright (c) 2023-present DeepGrace (complex dot invoke at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/deepgrace/unp
//

#ifndef ADDRESS_HPP
#define ADDRESS_HPP

#include <address_v4.hpp>
#include <address_v6.hpp>

namespace unp::ip
{
    class address
    {
    public:
        address() noexcept : type_(ipv4), ipv4_address_(), ipv6_address_()
        {
        }

        address(const unp::ip::address_v4& ipv4_address) noexcept : type_(ipv4), ipv4_address_(ipv4_address), ipv6_address_()
        {
        }

        address(const unp::ip::address_v6& ipv6_address) noexcept : type_(ipv6), ipv4_address_(), ipv6_address_(ipv6_address)
        {
        }

        address(const address& other) noexcept : type_(other.type_), ipv4_address_(other.ipv4_address_), ipv6_address_(other.ipv6_address_)
        {
        }

        address(address&& other) noexcept : type_(other.type_), ipv4_address_(other.ipv4_address_), ipv6_address_(other.ipv6_address_)
        {
        }

        address& operator=(const address& other) noexcept
        {
            type_ = other.type_;

            ipv4_address_ = other.ipv4_address_;
            ipv6_address_ = other.ipv6_address_;

            return *this;
        }

        address& operator=(address&& other) noexcept
        {
            type_ = other.type_;

            ipv4_address_ = other.ipv4_address_;
            ipv6_address_ = other.ipv6_address_;

            return *this;
        }

        address& operator=(const unp::ip::address_v4& ipv4_address) noexcept
        {
            type_ = ipv4;

            ipv4_address_ = ipv4_address;
            ipv6_address_ = unp::ip::address_v6();

            return *this;
        }

        address& operator=(const unp::ip::address_v6& ipv6_address) noexcept
        {
            type_ = ipv6;

            ipv4_address_ = unp::ip::address_v4();
            ipv6_address_ = ipv6_address;

            return *this;
        }

        bool is_v4() const noexcept
        {
            return type_ == ipv4;
        }

        bool is_v6() const noexcept
        {
            return type_ == ipv6;
        }

        unp::ip::address_v4 to_v4() const
        {
            if (type_ != ipv4)
                return {};

            return ipv4_address_;
        }

        unp::ip::address_v6 to_v6() const
        {
            if (type_ != ipv6)
                return {};

            return ipv6_address_;
        }

        std::string to_string() const
        {
            if (type_ == ipv6)
                return ipv6_address_.to_string();

            return ipv4_address_.to_string();
        }

        bool is_loopback() const noexcept
        {
            return is_v4() ? ipv4_address_.is_loopback() : ipv6_address_.is_loopback();
        }

        bool is_unspecified() const noexcept
        {
            return is_v4() ? ipv4_address_.is_unspecified() : ipv6_address_.is_unspecified();
        }

        bool is_multicast() const noexcept
        {
            return is_v4() ? ipv4_address_.is_multicast() : ipv6_address_.is_multicast();
        }

        friend bool operator==(const address& a1, const address& a2) noexcept
        {
            if (a1.type_ != a2.type_)
                return false;

            if (a1.type_ == address::ipv6)
                return a1.ipv6_address_ == a2.ipv6_address_;

            return a1.ipv4_address_ == a2.ipv4_address_;
        }

        friend bool operator!=(const address& a1, const address& a2) noexcept
        {
            return !(a1 == a2);
        }

        friend bool operator<(const address& a1, const address& a2) noexcept
        {
            if (a1.type_ < a2.type_)
                return true;

            if (a1.type_ > a2.type_)
                return false;

            if (a1.type_ == address::ipv6)
                return a1.ipv6_address_ < a2.ipv6_address_;

            return a1.ipv4_address_ < a2.ipv4_address_;
        }

        friend bool operator>(const address& a1, const address& a2) noexcept
        {
            return a2 < a1;
        }

        friend bool operator<=(const address& a1, const address& a2) noexcept
        {
            return !(a2 < a1);
        }

        friend bool operator>=(const address& a1, const address& a2) noexcept
        {
            return !(a1 < a2);
        }

    private:
        enum { ipv4, ipv6 } type_;

        unp::ip::address_v4 ipv4_address_;
        unp::ip::address_v6 ipv6_address_;
    };

    inline address make_address(const char* str)
    {
        unp::ip::address_v6 ipv6_address = unp::ip::make_address_v6(str);

        if (ipv6_address != unp::ip::address_v6())
            return address(ipv6_address);

        unp::ip::address_v4 ipv4_address = unp::ip::make_address_v4(str);

        if (ipv4_address != unp::ip::address_v4())
            return address(ipv4_address);

        return address();
    }

    inline address make_address(const std::string& str)
    {
        return make_address(str.c_str());
    }

    inline address make_address(std::string_view str)
    {
        return make_address(static_cast<std::string>(str));
    }

    template <typename S>
    constexpr decltype(auto) operator<<(S& s, const address& addr)
    {
        return s << addr.to_string();
    }
}

namespace std
{
    template <>
    struct hash<unp::ip::address>
    {
        std::size_t operator()(const unp::ip::address& addr) const noexcept
        {
            return addr.is_v4() ? std::hash<unp::ip::address_v4>()(addr.to_v4()) : std::hash<unp::ip::address_v6>()(addr.to_v6());
        }
    };
}

#endif
