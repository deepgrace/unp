//
// Copyright (c) 2023-present DeepGrace (complex dot invoke at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/deepgrace/unp
//

#include <deque>
#include <thread>
#include <iostream>
#include <unp.hpp>

// g++ -std=c++23 -Wall -O3 -Os -s -I example -I include example/echo_client_udp.cpp -o /tmp/echo_client_udp

namespace net = unp;

using udp = net::ip::udp;
using socket_t = udp::socket;

static constexpr int max_length = 1024;

using context_t = net::io_uring_context;
using echo_message_queue = std::deque<std::string>;

class echo_client
{
public:
    echo_client(context_t& ioc, const udp::endpoint& endpoint) :
    ioc(ioc), socket(ioc, udp::endpoint(udp(endpoint.protocol().family()), 0)), receiver(endpoint)
    {
        do_receive();
    }

    void write(const std::string& msg)
    {
        net::post(ioc, [this, msg]
        {
            bool write_in_progress = !send_msgs.empty();
            send_msgs.push_back(msg);

            if (!write_in_progress)
                do_send();
        });
    }

    void do_receive()
    {
        socket.async_receive_from(net::buffer(recv_msg, max_length), sender,
        [this](std::error_code ec, std::size_t bytes_transferred)
        {
            on_receive(ec, bytes_transferred);
        });
    }

    void on_receive(std::error_code ec, std::size_t length)
    {
        if (!ec)
        {
            std::cout.write(recv_msg, length);
            std::cout << std::endl;
        }
        else
        {
            if (ec != std::errc::no_message)
                std::cout << "async_receive_from " << ec.message() << std::endl;
        }

        do_receive();
    }

    void do_send()
    {
        auto& msg = send_msgs.front();

        socket.async_send_to(net::buffer(msg.data(), msg.length()), receiver,
        [this](std::error_code ec, std::size_t bytes_transferred)
        {
            on_send(ec, bytes_transferred);
        });
    }

    void on_send(std::error_code ec, std::size_t bytes_transferred)
    {
        if (ec)
        {
            if (ec != std::errc::no_message)
                std::cout << "async_send_to " << ec.message() << std::endl;

            std::cout << "server closed" << std::endl;

            return;
        }

        send_msgs.pop_front();

        if (!send_msgs.empty())
            do_send();
    }

private:
    context_t& ioc;
    socket_t socket;

    udp::endpoint sender;
    udp::endpoint receiver;

    char recv_msg[max_length];
    echo_message_queue send_msgs;
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

        std::thread t([&]{ ioc.run(source.get_token()); });

        net::scope_guard stop_on_exit = [&] noexcept
        {
            source.request_stop();
            t.join();
        };

        auto address = net::ip::make_address(argv[1]);
        echo_client c(ioc, udp::endpoint(address, std::atoi(argv[2])));

        std::string line;

        while (std::getline(std::cin, line))
        {
            if (!line.empty())
                c.write(line);
        }

    }
    catch (std::exception& e)
    {
        std::cerr << "Exception: " << e.what() << std::endl;
    }

    return 0;
}
