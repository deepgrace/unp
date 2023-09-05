//
// Copyright (c) 2023-present DeepGrace (complex dot invoke at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/deepgrace/unp
//

#include <thread>
#include <iostream>
#include <unp.hpp>

// g++ -std=c++23 -Wall -O3 -Os -s -I example -I include example/async_wait.cpp -o /tmp/async_wait

namespace net = unp;
using context_t = net::io_uring_context;

int main(int argc, char* argv[])
{
    context_t ctx;
    auto dur = std::chrono::seconds(3);

    net::steady_timer timer(ctx);
    net::inplace_stop_source source;

    timer.expires_at(timer.now() + dur);

    timer.async_wait([&](std::error_code ec)
    {
        if (!ec)
            std::cout << "elapsed" << std::endl;
        else if (ec == std::errc::operation_canceled)
            std::cout << ec.message() << std::endl;
    });

    std::thread t([&]{ ctx.run(source.get_token()); });
    std::this_thread::sleep_for(std::chrono::seconds(1));

    timer.expires_after(std::chrono::milliseconds(10));

    timer.async_wait([&](std::error_code ec)
    {
        if (!ec)
            std::cout << "later elapsed" << std::endl;
        else
            std::cout << "later " << ec.message() << std::endl;

        source.request_stop();
    });

    t.join();

    return 0;
}
