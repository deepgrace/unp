//
// Copyright (c) 2023-present DeepGrace (complex dot invoke at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/deepgrace/unp
//

#ifndef SAFE_FILE_DESCRIPTOR_HPP
#define SAFE_FILE_DESCRIPTOR_HPP

#include <utility>
#include <unistd.h>

namespace unp
{
    class safe_file_descriptor
    {
    public:
        safe_file_descriptor() noexcept
        {
        }

        explicit safe_file_descriptor(int fd) noexcept : fd_(fd)
        {
        }

        safe_file_descriptor(safe_file_descriptor&& other) noexcept : fd_(std::exchange(other.fd_, -1))
        {
        }

        safe_file_descriptor& operator=(safe_file_descriptor other) noexcept
        {
            std::swap(fd_, other.fd_);

            return *this;
        }

        bool valid() const noexcept
        {
            return fd_ >= 0;
        }

        int get() const noexcept
        {
            return fd_;
        }

        void reset(int fd)
        {
            fd_ = fd;
        }

        void close() noexcept
        {
            if (!valid())
                return;

            int result = ::close(std::exchange(fd_, -1));
            assert(result == 0);
        }

        ~safe_file_descriptor()
        {
            if (valid())
                close();
        }

    private:
        int fd_ = -1;
    };
}

#endif
