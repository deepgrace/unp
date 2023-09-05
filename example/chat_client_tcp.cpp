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
#include <chat_message.hpp>

// g++ -std=c++23 -Wall -O3 -Os -s -I example -I include example/chat_client_tcp.cpp -o /tmp/chat_client_tcp

namespace net = unp;

using tcp = net::ip::tcp;
using socket_t = tcp::socket;

using context_t = net::io_uring_context;
using chat_message_queue = std::deque<chat_message>;

class chat_client
{
public:
    chat_client(context_t& ioc, const tcp::endpoint& endpoint) : ioc(ioc), socket(ioc), endpoint(endpoint)
    {
        do_connect();
    }

    void write(const chat_message& msg)
    {
        net::post(ioc, [this, msg]
        {
            bool write_in_progress = !write_msgs.empty();
            write_msgs.push_back(msg);

            if (!write_in_progress)
                do_write();
        });
    }

    void close()
    {
        socket.close();
    }

    void do_connect()
    {
        net::async_connect(socket, endpoint,
        [this](std::error_code ec, int fd)
        {
            on_connect(ec, fd);
        });
    }

    void on_connect(std::error_code ec, int fd)
    {
        if (!ec)
            do_read_header();
        else
            std::cout << "async_connect " << ec.message() << std::endl;
    }

    void do_read_header()
    {
        net::async_read(socket, net::buffer(read_msg.data(), chat_message::header_length),
        [this](std::error_code ec, std::size_t bytes_transferred)
        {
            on_read_header(ec, bytes_transferred);
        });
    }

    void on_read_header(std::error_code ec, std::size_t bytes_transferred)
    {
        if (ec)
        {
            if (ec != std::errc::no_message)
                std::cout << "async_read " << ec.message() << std::endl;

            std::cout << "server closed" << std::endl;

            return;
        }

        if (read_msg.decode_header())
            do_read_body();
        else
            close();
    }

    void do_read_body()
    {
        net::async_read(socket, net::buffer(read_msg.body(), read_msg.body_length()),
        [this](std::error_code ec, std::size_t bytes_transferred)
        {
            on_read_body(ec, bytes_transferred);
        });
    }

    void on_read_body(std::error_code ec, std::size_t bytes_transferred)
    {
        if (ec)
        {
            if (ec != std::errc::no_message)
                std::cout << "async_read " << ec.message() << std::endl;

            std::cout << "server closed" << std::endl;

            return;
        }

        std::cout.write(read_msg.body(), read_msg.body_length());
        std::cout << std::endl;

        do_read_header();
    }

    void do_write()
    {
        auto& msg = write_msgs.front();

        net::async_write(socket, net::buffer(msg.data(), msg.length()),
        [this](std::error_code ec, std::size_t bytes_transferred)
        {
            on_write(ec, bytes_transferred);
        });
    }

    void on_write(std::error_code ec, std::size_t bytes_transferred)
    {
        if (ec)
        {
            if (ec != std::errc::no_message)
                std::cout << "async_write " << ec.message() << std::endl;

            std::cout << "server closed" << std::endl;

            return;
        }

        write_msgs.pop_front();

        if (!write_msgs.empty())
            do_write();
    }

private:
    context_t& ioc;
    socket_t socket;

    tcp::endpoint endpoint;

    chat_message read_msg;
    chat_message_queue write_msgs;
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
        chat_client c(ioc, tcp::endpoint(address, std::atoi(argv[2])));

        char line[chat_message::max_body_length + 1];

        while (std::cin.getline(line, chat_message::max_body_length + 1))
        {
            chat_message msg;
            msg.body_length(std::strlen(line));

            if (!msg.body_length())
                continue;

            std::memcpy(msg.body(), line, msg.body_length());
            msg.encode_header();

            c.write(msg);
        }

        c.close();
    }
    catch (std::exception& e)
    {
        std::cerr << "Exception: " << e.what() << std::endl;
    }

    return 0;
}
