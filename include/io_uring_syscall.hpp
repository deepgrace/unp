//
// Copyright (c) 2023-present DeepGrace (complex dot invoke at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/deepgrace/unp
//

#ifndef IO_URING_SYSCALL_HPP
#define IO_URING_SYSCALL_HPP

#include <errno.h>
#include <sys/syscall.h>

namespace unp
{
    template <typename F>
    int retry_interruptable_syscall(F&& f)
    {
        while (true)
        {
            auto ret = f();

            if (ret < 0)
            {
                auto err = errno;

                if (err == EINTR)
                    continue;
            }

            return ret;
        }
    }

    inline int io_uring_register(int fd, unsigned opcode, const void* arg, unsigned nr_args)
    {
        return retry_interruptable_syscall([=]
               {
                   return syscall(__NR_io_uring_register, fd, opcode, arg, nr_args);
               });
    }

    inline int io_uring_setup(unsigned entries, struct io_uring_params* p)
    {
        return retry_interruptable_syscall([=]
               {
                   return syscall(__NR_io_uring_setup, entries, p);
               });
    }

    inline int io_uring_enter(int fd, unsigned to_submit, unsigned min_complete, unsigned flags, sigset_t* sig)
    {
        return retry_interruptable_syscall([=]
               {
                   return syscall(__NR_io_uring_enter, fd, to_submit, min_complete, flags, sig, _NSIG / 8);
               });
    }
}

#endif
