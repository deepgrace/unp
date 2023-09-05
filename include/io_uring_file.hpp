//
// Copyright (c) 2023-present DeepGrace (complex dot invoke at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/deepgrace/unp
//

#ifndef IO_URING_FILE_HPP
#define IO_URING_FILE_HPP

#include <fcntl.h>

namespace unp
{
    class async_file
    {
    public:
        enum shutdown_type
        {
            shutdown_receive = SHUT_RD,
            shutdown_send = SHUT_WR,

            shutdown_both = SHUT_RDWR
        };

        explicit async_file(io_uring_context& context, int fd = -1) noexcept : context(context), fd_(fd)
        {
        }

        int get_fd()
        {
            return fd_.get();
        }

        io_uring_context& get_context()
        {
            return context;
        }

        bool is_open()
        {
            return fd_.valid();
        }

        void reset(int fd)
        {
            fd_.reset(fd);
        }

        void close()
        {
            fd_.close();
        }

        void shutdown(shutdown_type type)
        {
            ::shutdown(get_fd(), type);
        }

        io_uring_context& context;
        safe_file_descriptor fd_;
    };

    class file_base
    {
    public:
        enum seek_basis
        {
            seek_cur = SEEK_CUR,
            seek_set = SEEK_SET,

            seek_end = SEEK_END
        };

        enum flags
        {
            read_only = O_RDONLY,
            write_only = O_WRONLY,

            read_write = O_RDWR,
            append = O_APPEND,

            create = O_CREAT,
            exclusive = O_EXCL,

            truncate = O_TRUNC,
            close_on_exec = O_CLOEXEC,

            sync_all_on_write = O_SYNC
        };

        friend flags operator&(flags x, flags y)
        {
            return static_cast<flags>(static_cast<unsigned int>(x) & static_cast<unsigned int>(y));
        }

        friend flags operator|(flags x, flags y)
        {
            return static_cast<flags>(static_cast<unsigned int>(x) | static_cast<unsigned int>(y));
        }

        friend flags operator^(flags x, flags y)
        {
            return static_cast<flags>(static_cast<unsigned int>(x) ^ static_cast<unsigned int>(y));
        }

        friend flags operator~(flags x)
        {
            return static_cast<flags>(~static_cast<unsigned int>(x));
        }

        friend flags& operator&=(flags& x, flags y)
        {
            x = x & y;

            return x;
        }

        friend flags& operator|=(flags& x, flags y)
        {
            x = x | y;

            return x;
        }

        friend flags& operator^=(flags& x, flags y)
        {
            x = x ^ y;

            return x;
        }
    };

    class random_access_file : public file_base, public async_file
    {
    public:
        random_access_file(io_uring_context& context, const std::string& path, file_base::flags open_flags) : async_file(context)
        {
            int fd = open(path.c_str(), open_flags, 0644);

            if (fd < 0)
            {
                int errorCode = errno;
                throw(std::system_error(errorCode, std::system_category()));
            }

            reset(fd);
        }
    };

    class stream_file : public random_access_file
    {
    public:
        using random_access_file::random_access_file;
        offset_t offset = 0;
    };
}

#endif
