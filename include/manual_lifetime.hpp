//
// Copyright (c) 2023-present DeepGrace (complex dot invoke at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/deepgrace/unp
//

#ifndef MANUAL_LIFETIME_HPP
#define MANUAL_LIFETIME_HPP

#include <memory>
#include <type_traits>
#include <scope_guard.hpp>

namespace unp
{
    template <typename T>
    class manual_lifetime
    {
        static_assert(std::is_nothrow_destructible_v<T>);

    public:
        manual_lifetime() noexcept
        {
        }

        template <typename... Args>
        [[maybe_unused]] T& construct(Args&&... args) noexcept(std::is_nothrow_constructible_v<T, Args...>)
        {
            return *::new (static_cast<void*>(std::addressof(value_))) T(std::forward<Args>(args)...);
        }

        template <typename F>
        [[maybe_unused]] T& construct_with(F&& f) noexcept(std::is_nothrow_invocable_v<F>)
        {
            static_assert(std::is_same_v<std::invoke_result_t<F>, T>, "Return type of f() must be exactly T to permit copy-elision.");
            return *::new (static_cast<void*>(std::addressof(value_))) T(std::forward<F>(f)());
        }

        T& get() & noexcept
        {
            return value_;
        }

        T&& get() && noexcept
        {
            return std::forward<T>(value_);
        }

        const T& get() const & noexcept
        {
            return value_;
        }

        const T&& get() const && noexcept
        {
            return std::forward<const T>(value_);
        }

        void destruct() noexcept
        {
            value_.~T();
        }

        ~manual_lifetime()
        {
        }

    private:
        union
        {
            T value_;
        };
    };

    template <typename T>
    class manual_lifetime<T&>
    {
    public:
        manual_lifetime() noexcept : value_(nullptr)
        {
        }

        [[maybe_unused]] T& construct(T& value) noexcept
        {
            value_ = std::addressof(value);

            return value;
        }

        template <typename F>
        [[maybe_unused]] T& construct_with(F&& f) noexcept(std::is_nothrow_invocable_v<F>)
        {
            static_assert(std::is_same_v<std::invoke_result_t<F>, T&>);
            value_ = std::addressof(((F &&) f)());

            return get();
        }

        T& get() const noexcept
        {
            return *value_;
        }

        void destruct() noexcept
        {
        }

        ~manual_lifetime()
        {
        }

    private:
        T* value_;
    };

    template <typename T>
    class manual_lifetime<T&&>
    {
    public:
        manual_lifetime() noexcept : value_(nullptr)
        {
        }

        [[maybe_unused]] T&& construct(T&& value) noexcept
        {
            value_ = std::addressof(value);

            return std::forward<T>(value);
        }

        template <typename F>
        [[maybe_unused]] T&& construct_with(F&& f) noexcept(std::is_nothrow_invocable_v<F>)
        {
            static_assert(std::is_same_v<std::invoke_result_t<F>, T&&>);
            value_ = std::addressof(((F &&) f)());

            return get();
        }

        T&& get() const noexcept
        {
            return std::forward<T>(*value_);
        }

        void destruct() noexcept
        {
        }

        ~manual_lifetime()
        {
        }

    private:
        T* value_;
    };

    template <>
    class manual_lifetime<void>
    {
    public:
        manual_lifetime() noexcept = default;

        void construct() noexcept
        {
        }

        template <typename F>
        void construct_with(F&& f) noexcept(std::is_nothrow_invocable_v<F>)
        {
            static_assert(std::is_void_v<std::invoke_result_t<F>>);
            std::forward<F>(f)();
        }

        void get() const noexcept
        {
        }

        void destruct() noexcept
        {
        }

        ~manual_lifetime()
        {
        }
    };

    template <>
    class manual_lifetime<void const> : public manual_lifetime<void>
    {
    };

    template <typename T, typename... Args>
    [[maybe_unused]] T& activate_union_member(manual_lifetime<T>& box, Args&&... args) noexcept(std::is_nothrow_constructible_v<T, Args...>)
    {
        auto* p = ::new (&box) manual_lifetime<T>();
        scope_guard guard = [=] noexcept { p->~manual_lifetime(); };

        auto& t = box.construct(std::forward<Args>(args)...);
        guard.release();

        return t;
    }

    inline void activate_union_member(manual_lifetime<void>& box) noexcept
    {
        (::new (&box) manual_lifetime<void>())->construct();
    }

    template <typename T, typename F>
    [[maybe_unused]] T& activate_union_member_with(manual_lifetime<T>& box, F&& f) noexcept(std::is_nothrow_invocable_v<F>)
    {
        auto* p = ::new (&box) manual_lifetime<T>{};
        scope_guard guard = [=]() noexcept { p->~manual_lifetime(); };

        auto& t = p->construct_with(std::forward<F>(f));
        guard.release();

        return t;
    }

    template <typename T>
    void deactivate_union_member(manual_lifetime<T>& box) noexcept
    {
        box.destruct();
        box.~manual_lifetime();
    }
}

#endif
