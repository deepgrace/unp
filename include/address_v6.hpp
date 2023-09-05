//
// Copyright (c) 2023-present DeepGrace (complex dot invoke at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/deepgrace/unp
//

#ifndef ADDRESS_V6_HPP
#define ADDRESS_V6_HPP

#include <string>
#include <address_v4.hpp>

namespace unp::ip
{
    using scope_id_type = uint_least32_t;

    class address_v6
    {
    public:
        using bytes_type = std::array<unsigned char, 16>;

        address_v6() noexcept : addr_(), scope_id_(0)
        {
        }

        explicit address_v6(const bytes_type& bytes, scope_id_type scope_id = 0) : scope_id_(scope_id)
        {
            #if UCHAR_MAX > 0xFF
            for (std::size_t i = 0; i < bytes.size(); ++i)
                 if (bytes[i] > 0xFF)
                     throw(std::out_of_range("address_v6 from bytes_type"));
            #endif // UCHAR_MAX > 0xFF

            std::memcpy(addr_.s6_addr, bytes.data(), 16);
        }

        address_v6(const address_v6& other) noexcept : addr_(other.addr_), scope_id_(other.scope_id_)
        {
        }

        address_v6(address_v6&& other) noexcept : addr_(other.addr_), scope_id_(other.scope_id_)
        {
        }

        address_v6& operator=(const address_v6& other) noexcept
        {
            addr_ = other.addr_;
            scope_id_ = other.scope_id_;

            return *this;
        }

        address_v6& operator=(address_v6&& other) noexcept
        {
            addr_ = other.addr_;
            scope_id_ = other.scope_id_;

            return *this;
        }

        scope_id_type scope_id() const noexcept
        {
            return scope_id_;
        }

        void scope_id(scope_id_type id) noexcept
        {
            scope_id_ = id;
        }

        bytes_type to_bytes() const noexcept
        {
            bytes_type bytes;
            std::memcpy(bytes.data(), addr_.s6_addr, 16);

            return bytes;
        }

        std::string to_string() const
        {
            char buff[inet_ops::max_addr_v6_str_len];
            const char* addr = inet_ops::inet_ntop(AF_INET6, &addr_, buff, inet_ops::max_addr_v6_str_len, scope_id_);

            if (addr == 0)
                return {};

            return addr;
        }

        bool is_loopback() const noexcept
        {
            for (int i = 0; i < 15; ++i)
                 if (addr_.s6_addr[i] != 0)
                     return false;

            return addr_.s6_addr[15] == 1;
        }

        bool is_unspecified() const noexcept
        {
            for (int i = 0; i < 16; ++i)
                 if (addr_.s6_addr[i] != 0)
                     return false;

            return true;
        }

        bool is_link_local() const noexcept
        {
            return ((addr_.s6_addr[0] == 0xfe) && ((addr_.s6_addr[1] & 0xc0) == 0x80));
        }

        bool is_site_local() const noexcept
        {
            return ((addr_.s6_addr[0] == 0xfe) && ((addr_.s6_addr[1] & 0xc0) == 0xc0)); 
        }

        bool is_v4_mapped() const noexcept
        {
            for (int i = 0; i < 10; ++i)
                 if (addr_.s6_addr[i] != 0)
                     return false;

            return (addr_.s6_addr[10] == 0xff) && (addr_.s6_addr[11] == 0xff);

        }

        bool is_multicast() const noexcept
        {
            return addr_.s6_addr[0] == 0xff;
        }

        bool is_multicast_global() const noexcept
        {
            return ((addr_.s6_addr[0] == 0xff) && ((addr_.s6_addr[1] & 0x0f) == 0x0e));
        }

        bool is_multicast_link_local() const noexcept
        {
            return ((addr_.s6_addr[0] == 0xff) && ((addr_.s6_addr[1] & 0x0f) == 0x02));
        }

        bool is_multicast_node_local() const noexcept
        {
            return ((addr_.s6_addr[0] == 0xff) && ((addr_.s6_addr[1] & 0x0f) == 0x01));
        }

        bool is_multicast_org_local() const noexcept
        {
            return ((addr_.s6_addr[0] == 0xff) && ((addr_.s6_addr[1] & 0x0f) == 0x08));
        }

        bool is_multicast_site_local() const noexcept
        {
            return ((addr_.s6_addr[0] == 0xff) && ((addr_.s6_addr[1] & 0x0f) == 0x05));
        }

        friend bool operator==(const address_v6& a1, const address_v6& a2) noexcept
        {
            return std::memcmp(&a1.addr_, &a2.addr_, sizeof(in6_addr)) == 0 && a1.scope_id_ == a2.scope_id_;
        }

        friend bool operator!=(const address_v6& a1, const address_v6& a2) noexcept
        {
            return !(a1 == a2);
        }

        friend bool operator<(const address_v6& a1, const address_v6& a2) noexcept
        {
            int memcmp_result = std::memcmp(&a1.addr_, &a2.addr_, sizeof(in6_addr));

            if (memcmp_result < 0)
                return true;

            if (memcmp_result > 0)
                return false;

            return a1.scope_id_ < a2.scope_id_;
        }

        friend bool operator>(const address_v6& a1, const address_v6& a2) noexcept
        {
            return a2 < a1;
        }

        friend bool operator<=(const address_v6& a1, const address_v6& a2) noexcept
        {
            return !(a2 < a1);
        }

        friend bool operator>=(const address_v6& a1, const address_v6& a2) noexcept
        {
            return !(a1 < a2);
        }

        static address_v6 any() noexcept
        {
            return address_v6();
        }

        static address_v6 loopback() noexcept
        {
            address_v6 addr;
            addr.addr_.s6_addr[15] = 1;

            return addr;
        }

    private:
        in6_addr addr_;
        scope_id_type scope_id_;
    };

    inline address_v6 make_address_v6(const address_v6::bytes_type& bytes, scope_id_type scope_id = 0)
    {
        return address_v6(bytes, scope_id);
    }

    inline address_v6 make_address_v6(const char* str)
    {
        unsigned long scope_id = 0;
        address_v6::bytes_type bytes;

        if (inet_ops::inet_pton(AF_INET6, str, &bytes[0], &scope_id) <= 0)
            return address_v6();

        return address_v6(bytes, scope_id);
    }

    inline address_v6 make_address_v6(const std::string& str)
    {
        return make_address_v6(str.c_str());
    }

    inline address_v6 make_address_v6(std::string_view str)
    {
        return make_address_v6(static_cast<std::string>(str));
    }

    enum v4_mapped_t { v4_mapped };

    inline address_v4 make_address_v4(v4_mapped_t, const address_v6& v6_addr)
    {
        if (!v6_addr.is_v4_mapped())
            return {};

        address_v6::bytes_type v6_bytes = v6_addr.to_bytes();
        address_v4::bytes_type v4_bytes = { { v6_bytes[12], v6_bytes[13], v6_bytes[14], v6_bytes[15] } };

        return address_v4(v4_bytes);
    }

    inline address_v6 make_address_v6(v4_mapped_t, const address_v4& v4_addr)
    {
        address_v4::bytes_type v4_bytes = v4_addr.to_bytes();
        address_v6::bytes_type v6_bytes = { { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0xFF, 0xFF, v4_bytes[0], v4_bytes[1], v4_bytes[2], v4_bytes[3] } };

        return address_v6(v6_bytes);
    }

    template <typename S>
    constexpr decltype(auto) operator<<(S& s, const address_v6& addr)
    {
        return s << addr.to_string();
    }
}

namespace std
{
    template <>
    struct hash<unp::ip::address_v6>
    {
        std::size_t operator()(const unp::ip::address_v6& addr) const noexcept
        {
            const unp::ip::address_v6::bytes_type bytes = addr.to_bytes();
            std::size_t result = static_cast<std::size_t>(addr.scope_id());

            combine_4_bytes(result, &bytes[0]);
            combine_4_bytes(result, &bytes[4]);

            combine_4_bytes(result, &bytes[8]);
            combine_4_bytes(result, &bytes[12]);

            return result;
        }

    private:
        static void combine_4_bytes(std::size_t& seed, const unsigned char* bytes)
        {
            const std::size_t bytes_hash = (static_cast<std::size_t>(bytes[0]) << 24) | (static_cast<std::size_t>(bytes[1]) << 16) |
                                           (static_cast<std::size_t>(bytes[2]) << 8) | (static_cast<std::size_t>(bytes[3]));

            seed ^= bytes_hash + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        }
    };
}

#endif
