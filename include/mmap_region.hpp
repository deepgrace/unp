//
// Copyright (c) 2023-present DeepGrace (complex dot invoke at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/deepgrace/unp
//

#ifndef MMAP_REGION_HPP
#define MMAP_REGION_HPP

#include <utility>
#include <sys/mman.h>

namespace unp
{
    struct mmap_region
    {
        mmap_region() noexcept
        {
        }

        explicit mmap_region(void* data, std::size_t size) noexcept : data_(data), size_(size)
        {
        }

        mmap_region(mmap_region&& r) noexcept : data_(std::exchange(r.data_, nullptr)), size_(std::exchange(r.size_, 0))
        {
        }

        mmap_region& operator=(mmap_region r) noexcept
        {
            std::swap(data_, r.data_);
            std::swap(size_, r.size_);

            return *this;
        }

        void* data() const noexcept
        {
            return data_;
        }

        std::size_t size() const noexcept
        {
            return size_;
        }

        ~mmap_region()
        {
            if (size_ > 0)
                munmap(data_, size_);
        }

    private:
        void* data_ = nullptr;
        std::size_t size_ = 0;
    };
}

#endif
