//
// Copyright (c) 2023-present DeepGrace (complex dot invoke at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/deepgrace/unp
//

#ifndef IO_URING_BUFFER_HPP
#define IO_URING_BUFFER_HPP

#include <span>

namespace unp
{
    template <typename T>
    constexpr decltype(auto) buffer(T* data, std::size_t size)
    {
        return std::as_writable_bytes(std::span((char*)const_cast<std::remove_cvref_t<T>*>(data), size));
    }

    template <typename T>
    requires requires { std::declval<T>().data(); std::declval<T>().size(); }
    constexpr decltype(auto) buffer(T&& t)
    {
        return buffer(std::forward<T>(t).data(), std::forward<T>(t).size());
    }

    template <typename T>
    constexpr decltype(auto) advance(T&& t, std::size_t n)
    {
        return buffer(std::forward<T>(t).data() + n, std::forward<T>(t).size() - n);
    }
}

#endif
