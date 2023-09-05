//
// Copyright (c) 2023-present DeepGrace (complex dot invoke at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/deepgrace/unp
//

#ifndef ADDRESS_V4_HPP
#define ADDRESS_V4_HPP

#include <array>
#include <arpa/inet.h>
#include <string_view>
#include <sys/socket.h>
#include <inet_ops.hpp>

namespace unp::ip
{
    class address_v4
    {
    public:
        using uint_type = uint_least32_t;
        using bytes_type = std::array<unsigned char, 4>;

        address_v4() noexcept
        {
            addr_.s_addr = 0;
        }

        explicit address_v4(uint_type addr)
        {
            if (std::numeric_limits<uint_type>::max() > 0xFFFFFFFF)
                throw(std::out_of_range("address_v4 from unsigned integer"));

            addr_.s_addr = htonl(static_cast<uint32_t>(addr));
        }

        explicit address_v4(const bytes_type& bytes)
        {
            #if UCHAR_MAX > 0xFF
            if (bytes[0] > 0xFF || bytes[1] > 0xFF || bytes[2] > 0xFF || bytes[3] > 0xFF)
                std::throw(std::out_of_range("address_v4 from bytes_type"));
            #endif

            std::memcpy(&addr_.s_addr, bytes.data(), 4);
        }

        address_v4(const address_v4& other) noexcept : addr_(other.addr_)
        {
        }

        address_v4(address_v4&& other) noexcept : addr_(other.addr_)
        {
        }

        address_v4& operator=(const address_v4& other) noexcept
        {
            addr_ = other.addr_;

            return *this;
        }

        address_v4& operator=(address_v4&& other) noexcept
        {
            addr_ = other.addr_;

            return *this;
        }

        bytes_type to_bytes() const noexcept
        {
            bytes_type bytes;
            std::memcpy(bytes.data(), &addr_.s_addr, 4);

            return bytes;
        }

        uint_type to_uint() const noexcept
        {
            return ntohl(addr_.s_addr);
        }

        std::string to_string() const
        {
            char buff[inet_ops::max_addr_v4_str_len];
            const char* addr = inet_ops::inet_ntop(AF_INET, &addr_, buff, inet_ops::max_addr_v4_str_len);

            if (addr == 0)
                return {};

            return addr;
        }

        bool is_loopback() const noexcept
        {
            return (to_uint() & 0xFF000000) == 0x7F000000;
        }

        bool is_unspecified() const noexcept
        {
            return to_uint() == 0;
        }

        bool is_multicast() const noexcept
        {
            return (to_uint() & 0xF0000000) == 0xE0000000;
        }

        friend bool operator==(const address_v4& a1, const address_v4& a2) noexcept
        {
            return a1.addr_.s_addr == a2.addr_.s_addr;
        }

        friend bool operator!=(const address_v4& a1, const address_v4& a2) noexcept
        {
            return a1.addr_.s_addr != a2.addr_.s_addr;
        }

        friend bool operator<(const address_v4& a1, const address_v4& a2) noexcept
        {
            return a1.to_uint() < a2.to_uint();
        }

        friend bool operator>(const address_v4& a1, const address_v4& a2) noexcept
        {
            return a1.to_uint() > a2.to_uint();
        }

        friend bool operator<=(const address_v4& a1, const address_v4& a2) noexcept
        {
            return a1.to_uint() <= a2.to_uint();
        }

        friend bool operator>=(const address_v4& a1, const address_v4& a2) noexcept
        {
            return a1.to_uint() >= a2.to_uint();
        }

        static address_v4 any() noexcept
        {
            return address_v4();
        }

        static address_v4 loopback() noexcept
        {
            return address_v4(0x7F000001);
        }

        static address_v4 broadcast() noexcept
        {
            return address_v4(0xFFFFFFFF);
        }

    private:
        in_addr addr_;
    };

    inline address_v4 make_address_v4(const address_v4::bytes_type& bytes)
    {
        return address_v4(bytes);
    }

    inline address_v4 make_address_v4(address_v4::uint_type addr)
    {
        return address_v4(addr);
    }

    inline address_v4 make_address_v4(const char* str)
    {
        address_v4::bytes_type bytes;

        if (inet_ops::inet_pton(AF_INET, str, &bytes, 0) <= 0)
            return address_v4();

        return address_v4(bytes);
    }

    inline address_v4 make_address_v4(const std::string& str)
    {
        return make_address_v4(str.c_str());
    }

    inline address_v4 make_address_v4(std::string_view str)
    {
        return make_address_v4(static_cast<std::string>(str));
    }

    template <typename S>
    constexpr decltype(auto) operator<<(S& s, const address_v4& addr)
    {
        return s << addr.to_string();
    }
}

namespace std
{
    template <>
    struct hash<unp::ip::address_v4>
    {
        std::size_t operator()(const unp::ip::address_v4& addr) const noexcept
        {
            return std::hash<unsigned int>()(addr.to_uint());
        }
    };
}

#endif
