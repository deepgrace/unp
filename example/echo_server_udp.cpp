//
// Copyright (c) 2023-present DeepGrace (complex dot invoke at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/deepgrace/unp
//

#include <iostream>
#include <unp.hpp>

// g++ -std=c++23 -Wall -O3 -Os -s -I example -I include example/echo_server_udp.cpp -o /tmp/echo_server_udp

namespace net = unp;

using udp = net::ip::udp;
using socket_t = udp::socket;

static constexpr int max_length = 1024;
using context_t = net::io_uring_context;

class echo_server
{
public:
    echo_server(context_t& ioc, const udp::endpoint& endpoint) : socket(ioc, endpoint)
    {
        do_receive();
    }

    void do_receive()
    {
        net::async_receive_from(socket, net::buffer(data, max_length), sender,
        [this](std::error_code ec, std::size_t bytes_transferred)
        {
            on_receive(ec, bytes_transferred);
        });
    }

    void on_receive(std::error_code ec, std::size_t length)
    {
        if (!ec)
            do_send(length);
        else
        {
            if (ec != std::errc::no_message)
                std::cout << "async_receive_from " << ec.message() << std::endl;

            do_receive();
        }
    }

    void do_send(std::size_t length)
    {
        net::async_send_to(socket, net::buffer(data, length), sender,
        [this](std::error_code ec, std::size_t bytes_transferred)
        {
            on_send(ec, bytes_transferred);
        });
    }

    void on_send(std::error_code ec, std::size_t bytes_transferred)
    {
        if (!ec)
            do_receive();
        else
        {
            if (ec != std::errc::no_message)
                std::cout << "async_send_to " << ec.message() << std::endl;
        }
    }

private:
    socket_t socket;

    udp::endpoint sender;
    char data[max_length];
};

int main(int argc, char* argv[])
{
    try
    {
        if (argc != 3)
        {
            std::cerr << "Usage: " << argv[0] << " <host> <port>" << std::endl;

            return 1;
        }

        context_t ioc;
        net::inplace_stop_source source;

        auto address = net::ip::make_address(argv[1]);
        echo_server s(ioc, udp::endpoint(address, std::atoi(argv[2])));

        ioc.run(source.get_token());
    }
    catch (std::exception& e)
    {
        std::cerr << "Exception: " << e.what() << std::endl;
    }

    return 0;
}
