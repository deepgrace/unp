//
// Copyright (c) 2023-present DeepGrace (complex dot invoke at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/deepgrace/unp
//

#ifndef IO_URING_CONTEXT_HPP
#define IO_URING_CONTEXT_HPP

#include <cstring>
#include <cassert>
#include <signal.h>
#include <functional>
#include <arpa/inet.h>
#include <sys/eventfd.h>
#include <mmap_region.hpp>
#include <scope_guard.hpp>
#include <intrusive_heap.hpp>
#include <io_uring_buffer.hpp>
#include <io_uring_syscall.hpp>
#include <safe_file_descriptor.hpp>
#include <atomic_intrusive_queue.hpp>

namespace unp
{
    class io_uring_context
    {
    public:
        static constexpr int map_prot = PROT_READ | PROT_WRITE;
        static constexpr int map_flag = MAP_SHARED | MAP_POPULATE;

        io_uring_context()
        {
            init_params();

            init_cq();
            init_sq();

            init_sqe();
            init_event();
        }

        void init_params()
        {
            std::memset(&params, 0, sizeof(params));
            int fd = io_uring_setup(256, &params);

            if (fd < 0)
                throw(std::system_error(-fd, std::system_category()));

            ring_fd = safe_file_descriptor(fd);
        }

        void init_cq()
        {
            auto cq_len = params.cq_off.cqes + params.cq_entries * sizeof(io_uring_cqe);
            auto cq_ptr = mmap(0, cq_len, map_prot, map_flag, ring_fd.get(), IORING_OFF_CQ_RING);

            if (cq_ptr == MAP_FAILED)
            {
                int error_code = errno;
                throw(std::system_error(error_code, std::system_category()));
            }

            cq_mmap = mmap_region(cq_ptr, cq_len);
            char* cq_block = static_cast<char*>(cq_ptr);

            cq_entry_count = params.cq_entries;
            assert(cq_entry_count == *reinterpret_cast<unsigned*>(cq_block + params.cq_off.ring_entries));

            cq_mask = *reinterpret_cast<unsigned*>(cq_block + params.cq_off.ring_mask);
            assert(cq_mask == (cq_entry_count - 1));

            cq_head = reinterpret_cast<std::atomic<unsigned>*>(cq_block + params.cq_off.head);
            cq_tail = reinterpret_cast<std::atomic<unsigned>*>(cq_block + params.cq_off.tail);

            cq_entries = reinterpret_cast<io_uring_cqe*>(cq_block + params.cq_off.cqes);
            cq_overflow = reinterpret_cast<std::atomic<unsigned>*>(cq_block + params.cq_off.overflow);
        }

        void init_sq()
        {
            auto sq_len = params.sq_off.array + params.sq_entries * sizeof(__u32);
            auto sq_ptr = mmap(0, sq_len, map_prot, map_flag, ring_fd.get(), IORING_OFF_SQ_RING);

            if (sq_ptr == MAP_FAILED)
            {
                int error_code = errno;
                throw(std::system_error(error_code, std::system_category()));
            }

            sq_mmap = mmap_region(sq_ptr, sq_len);
            char* sq_block = static_cast<char*>(sq_ptr);

            sq_entry_count = params.sq_entries;
            assert(sq_entry_count == *reinterpret_cast<unsigned*>(sq_block + params.sq_off.ring_entries));

            sq_mask = *reinterpret_cast<unsigned*>(sq_block + params.sq_off.ring_mask);
            assert(sq_mask == (sq_entry_count - 1));

            sq_head = reinterpret_cast<std::atomic<unsigned>*>(sq_block + params.sq_off.head);
            sq_tail = reinterpret_cast<std::atomic<unsigned>*>(sq_block + params.sq_off.tail);

            sq_flags = reinterpret_cast<std::atomic<unsigned>*>(sq_block + params.sq_off.flags);
            sq_dropped = reinterpret_cast<std::atomic<unsigned>*>(sq_block + params.sq_off.dropped);

            sq_index_array = reinterpret_cast<unsigned*>(sq_block + params.sq_off.array);
        }

        void init_sqe()
        {
            auto sqe_len = params.sq_entries * sizeof(io_uring_sqe);
            auto sqe_ptr = mmap(0, sqe_len, map_prot, map_flag, ring_fd.get(), IORING_OFF_SQES);

            if (sqe_ptr == MAP_FAILED)
            {
                int error_code = errno;
                throw(std::system_error(error_code, std::system_category()));
            }

            sqe_mmap = mmap_region(sqe_ptr, sqe_len);
            sq_entries = reinterpret_cast<io_uring_sqe*>(sqe_ptr);
        }

        void init_event()
        {
            int fd = eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK);

            if (fd < 0)
            {
                int error_code = errno;
                throw(std::system_error(error_code, std::system_category()));
            }

            remote_queue_event_fd = safe_file_descriptor(fd);
        }

        template <typename T>
        void run(T t)
        {
            stop_operation stop_op;
            auto upon_stop = [&]{ schedule_impl(&stop_op); };

            typename T::template callback_type<decltype(upon_stop)> stop_callback(std::move(t), std::move(upon_stop));
            run_impl(stop_op.should_stop);
        }

        using operation_queue = intrusive_queue<operation_base, &operation_base::next>;
        using timer_heap = intrusive_heap<timer_operation, &timer_operation::next,
                           &timer_operation::prev, time_point, &timer_operation::duetime>;

        bool is_running_on_io_thread() const noexcept
        {
            return this == current_thread_context;
        }

        void run_impl(const bool& should_stop)
        {
            auto* old_context = std::exchange(current_thread_context, this);

            scope_guard g = [=] noexcept
            {
                std::exchange(current_thread_context, old_context);
            };

            while (true)
            {
                execute_pending_local();

                if (should_stop)
                    break;

                acquire_completion_queue_items();

                if (timers_are_dirty)
                    update_timers();

                if (!remote_queue_read_submitted)
                    acquire_remote_queued_items();

                while (!pending_io_queue.empty() && can_submit_io())
                {
                    auto* item = pending_io_queue.pop_front();
                    item->execute_(item);
                }

                if (local_queue.empty() || sq_unflushed_count > 0)
                {
                    const bool is_idle = sq_unflushed_count == 0 && local_queue.empty();

                    if (is_idle && !remote_queue_read_submitted)
                        remote_queue_read_submitted = register_remote_queue_notification();

                    unsigned flags = 0;
                    int min_completion_count = 0;

                    if (is_idle && (remote_queue_read_submitted || pending_operation_count() == cq_entry_count))
                    {
                        min_completion_count = 1;
                        flags = IORING_ENTER_GETEVENTS;
                    }

                    int result = io_uring_enter(ring_fd.get(), sq_unflushed_count, min_completion_count, flags, nullptr);

                    if (result < 0)
                    {
                        int error_code = errno;
                        throw(std::system_error(error_code, std::system_category()));
                    }

                    sq_unflushed_count -= result;
                    cq_pending_count += result;
                }
            }
        }

        void schedule_impl(operation_base* op)
        {
            assert(op != nullptr);

            if (is_running_on_io_thread())
                schedule_local(op);
            else
                schedule_remote(op);
        }

        void schedule_local(operation_base* op) noexcept
        {
            local_queue.push_back(op);
        }

        void schedule_local(operation_queue ops) noexcept
        {
            local_queue.append(std::move(ops));
        }

        void schedule_remote(operation_base* op) noexcept
        {
            bool io_thread_was_inactive = remote_queue_.enqueue(op);

            if (io_thread_was_inactive)
                signal_remote_queue();
        }

        void schedule_pending_io(operation_base* op) noexcept
        {
            assert(is_running_on_io_thread());
            pending_io_queue.push_back(op);
        }

        void reschedule_pending_io(operation_base* op) noexcept
        {
            assert(is_running_on_io_thread());
            pending_io_queue.push_front(op);
        }

        void insert_timer(timer_operation* op) noexcept
        {
            assert(is_running_on_io_thread());
            timers.insert(op);

            if (timers.top() == op)
                timers_are_dirty = true;
        }

        void execute_pending_local() noexcept
        {
            if (local_queue.empty())
                return;

            size_t count = 0;
            auto pending = std::move(local_queue);

            while (!pending.empty())
            {
                auto* item = pending.pop_front();

                item->execute_(item);
                ++count;
            }
        }

        void acquire_completion_queue_items() noexcept
        {
            std::uint32_t head = cq_head->load(std::memory_order_relaxed);
            std::uint32_t tail = cq_tail->load(std::memory_order_acquire);

            if (head != tail)
            {
                const auto mask = cq_mask;
                const auto count = tail - head;

                assert(count <= cq_entry_count);
                operation_queue completion_queue;

                for (std::uint32_t i = 0; i < count; ++i)
                {
                     auto& cqe = cq_entries[(head + i) & mask];

                     if (cqe.user_data == remote_queue_event_user_data)
                     {
                         if (cqe.res < 0)
                             std::terminate();

                         __u64 buffer;
                         ssize_t bytes_read = read(remote_queue_event_fd.get(), &buffer, sizeof(buffer));

                         if (bytes_read < 0)
                             std::terminate();

                         assert(bytes_read == sizeof(buffer));
                         remote_queue_read_submitted = false;

                         continue;
                     }
                     else if (cqe.user_data == timer_user_data())
                     {
                         assert(active_timer_count > 0);
                         --active_timer_count;

                         if (cqe.res != ECANCELED)
                             timers_are_dirty = true;

                         if (active_timer_count == 0)
                             current_duetime.reset();

                         continue;
                     }
                     else if (cqe.user_data == remove_timer_user_data())
                         continue;

                     auto& completion_state = *reinterpret_cast<completion_base*>(static_cast<std::uintptr_t>(cqe.user_data));

                     completion_state.result = cqe.res;
                     completion_queue.push_back(&completion_state);
                }

                schedule_local(std::move(completion_queue));

                cq_head->store(tail, std::memory_order_release);
                cq_pending_count -= count;
            }
        }

        void acquire_remote_queued_items() noexcept
        {
            assert(!remote_queue_read_submitted);

            auto items = remote_queue_.dequeue_all();
            schedule_local(std::move(items));
        }

        bool register_remote_queue_notification() noexcept
        {
            auto fill = [this](io_uring_sqe& sqe) noexcept
            {
                auto queued_items = remote_queue_.mark_inactive_or_dequeue_all();

                if (!queued_items.empty())
                {
                    schedule_local(std::move(queued_items));

                    return false;
                }

                sqe.opcode = IORING_OP_POLL_ADD;
                sqe.fd = remote_queue_event_fd.get();

                sqe.poll_events = POLL_IN;
                sqe.user_data = remote_queue_event_user_data;

                return true;
            };

            if (submit_io(fill))
                return true;

            return false;
        }

        void signal_remote_queue()
        {
            const __u64 value = 1;
            ssize_t bytes_written = write(remote_queue_event_fd.get(), &value, sizeof(value));

            if (bytes_written < 0)
            {
                int error_code = errno;
                throw(std::system_error(error_code, std::system_category()));
            }

            assert(bytes_written == sizeof(value));
        }

        void remove_timer(timer_operation* op) noexcept
        {
            assert(!timers.empty());

            if (timers.top() == op)
                timers_are_dirty = true;

            timers.remove(op);
        }

        void update_timers() noexcept
        {
            if (!timers.empty())
            {
                time_point now = monotonic_clock::now();

                while (!timers.empty() && timers.top()->duetime <= now)
                {
                    timer_operation* item = timers.pop();

                    if (item->cancelable)
                    {
                        auto old_state = item->state.fetch_add(timer_operation::timer_elapsed_flag, std::memory_order_acq_rel);

                        if ((old_state & timer_operation::cancel_pending_flag) != 0)
                             continue;
                    }

                    schedule_local(item);
                }
            }

            if (timers.empty())
            {
                if (current_duetime.has_value() && submit_timer_cancel())
                {
                    current_duetime.reset();
                    timers_are_dirty = false;
                }
            }
            else
            {
                const auto earliest_duetime = timers.top()->duetime;

                if (current_duetime)
                {
                    constexpr auto threshold = std::chrono::microseconds(1);

                    if (earliest_duetime < (*current_duetime - threshold))
                    {
                        if (submit_timer_cancel())
                        {
                            current_duetime.reset();

                            if (submit_timer(earliest_duetime))
                            {
                                current_duetime = earliest_duetime;
                                timers_are_dirty = false;
                            }
                        }
                    }
                    else
                        timers_are_dirty = false;
                }
                else if (submit_timer(earliest_duetime))
                {
                    current_duetime = earliest_duetime;
                    timers_are_dirty = false;
                }
            }
        }

        bool submit_timer(const time_point& duetime) noexcept
        {
            auto fill = [&](io_uring_sqe& sqe) noexcept
            {
                sqe.opcode = IORING_OP_TIMEOUT;
                sqe.addr = reinterpret_cast<std::uintptr_t>(&time_);

                sqe.len = 1;

                sqe.timeout_flags = IORING_TIMEOUT_ABS;
                sqe.user_data = timer_user_data();

                time_.tv_sec = duetime.seconds_part();
                time_.tv_nsec = duetime.nanoseconds_part();
            };

            if (submit_io(fill))
            {
                ++active_timer_count;

                return true;
            }

            return false;
        }

        bool submit_timer_cancel() noexcept
        {
            auto fill = [&](io_uring_sqe& sqe) noexcept
            {
                sqe.opcode = IORING_OP_TIMEOUT_REMOVE;

                sqe.addr = timer_user_data();
                sqe.user_data = remove_timer_user_data();
            };

            return submit_io(fill);
        }

        template <typename F>
        bool submit_io(F fill) noexcept
        {
            assert(is_running_on_io_thread());

            if (pending_operation_count() < cq_entry_count)
            {
                const auto head = sq_head->load(std::memory_order_acquire);
                const auto tail = sq_tail->load(std::memory_order_relaxed);

                const auto used_count = tail - head;
                assert(used_count <= sq_entry_count);

                if (used_count < sq_entry_count)
                {
                    const auto index = tail & sq_mask;
                    auto& sqe = sq_entries[index];

                    static_assert(noexcept(fill(sqe)));
                    std::memset(&sqe, 0, sizeof(sqe));

                    if constexpr(std::is_void_v<decltype(fill(sqe))>)
                        fill(sqe);
                    else if (!fill(sqe))
                        return false;

                    sq_index_array[index] = index;
                    sq_tail->store(tail + 1, std::memory_order_release);

                    ++sq_unflushed_count;

                    return true;
                }
            }

            return false;
        }

        std::uint32_t pending_operation_count() const noexcept
        {
            return cq_pending_count + sq_unflushed_count;
        }

        bool can_submit_io() const noexcept
        {
            return sq_unflushed_count < sq_entry_count && pending_operation_count() < cq_entry_count;
        }

        std::uintptr_t timer_user_data() const
        {
            return reinterpret_cast<std::uintptr_t>(&timers);
        }

        std::uintptr_t remove_timer_user_data() const
        {
            return reinterpret_cast<std::uintptr_t>(&current_duetime);
        }

        ~io_uring_context()
        {
        }

        std::uint32_t sq_mask;
        std::uint32_t sq_entry_count;

        unsigned* sq_index_array;
        io_uring_sqe* sq_entries;

        std::atomic<unsigned>* sq_tail;
        const std::atomic<unsigned>* sq_head;

        std::atomic<unsigned>* sq_flags;
        std::atomic<unsigned>* sq_dropped;

        std::uint32_t cq_mask;
        std::uint32_t cq_entry_count;

        io_uring_cqe* cq_entries;
        const std::atomic<unsigned>* cq_overflow;

        std::atomic<unsigned>* cq_head;
        const std::atomic<unsigned>* cq_tail;

        safe_file_descriptor ring_fd;
        safe_file_descriptor remote_queue_event_fd;

        mmap_region cq_mmap;
        mmap_region sq_mmap;

        mmap_region sqe_mmap;
        io_uring_params params;

        operation_queue local_queue;
        operation_queue pending_io_queue;

        timer_heap timers;
        std::optional<time_point> current_duetime;

        std::uint32_t sq_unflushed_count = 0;
        std::uint32_t cq_pending_count = 0;

        bool timers_are_dirty = false;
        bool remote_queue_read_submitted = false;

        std::uint32_t active_timer_count = 0;

        __kernel_timespec time_;
        atomic_intrusive_queue<operation_base, &operation_base::next> remote_queue_;
    };
}

#endif
