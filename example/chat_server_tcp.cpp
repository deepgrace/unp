//
// Copyright (c) 2023-present DeepGrace (complex dot invoke at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/deepgrace/unp
//

#include <set>
#include <list>
#include <deque>
#include <iostream>
#include <unp.hpp>
#include <chat_message.hpp>

// g++ -std=c++23 -Wall -O3 -Os -s -I example -I include example/chat_server_tcp.cpp -o /tmp/chat_server_tcp

namespace net = unp;

using tcp = net::ip::tcp;
using socket_t = tcp::socket;

using context_t = net::io_uring_context;
using chat_message_queue = std::deque<chat_message>;

class chat_participant
{
public:
    virtual ~chat_participant(){}
    virtual void deliver(const chat_message& msg) = 0;
};

using chat_participant_ptr = std::shared_ptr<chat_participant>;

class chat_room
{
public:
    void join(chat_participant_ptr participant)
    {
        participants.insert(participant);

        for (auto& msg: recent_msgs)
             participant->deliver(msg);
    }

    void leave(chat_participant_ptr participant)
    {
        participants.erase(participant);
    }

    void deliver(const chat_message& msg, uint64_t self)
    {
        recent_msgs.push_back(msg);

        while (recent_msgs.size() > max_recent_msgs)
               recent_msgs.pop_front();

        for (auto& participant: participants)
             //if (self != uint64_t(participant.get()))
                 participant->deliver(msg);
    }

private:
    chat_message_queue recent_msgs;

    static constexpr int max_recent_msgs = 100;
    std::set<chat_participant_ptr> participants;
};

class chat_session : public chat_participant, public std::enable_shared_from_this<chat_session>
{
public:
    chat_session(socket_t socket, chat_room& room) : socket(std::move(socket)), room(room)
    {
    }

    void start()
    {
        room.join(shared_from_this());
        do_read_header();
    }

    void deliver(const chat_message& msg)
    {
        bool write_in_progress = !write_msgs.empty();
        write_msgs.push_back(msg);

        if (!write_in_progress)
            do_write();
    }

private:
    void do_read_header()
    {
        net::async_read(socket, net::buffer(read_msg.data(), chat_message::header_length),
        [this, self = shared_from_this()](std::error_code ec, std::size_t bytes_transferred)
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

            room.leave(shared_from_this());

            return;
        }
 
        if (read_msg.decode_header())
            do_read_body();
        else
            room.leave(shared_from_this());
    }

    void do_read_body()
    {
        net::async_read(socket, net::buffer(read_msg.body(), read_msg.body_length()),
        [this, self = shared_from_this()](std::error_code ec, std::size_t bytes_transferred)
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

            room.leave(shared_from_this());

            return;
        }
 
        room.deliver(read_msg, self);
        do_read_header();
    }

    void do_write()
    {
        auto& msg = write_msgs.front();

        net::async_write(socket, net::buffer(msg.data(), msg.length()),
        [this, self = shared_from_this()](std::error_code ec, std::size_t bytes_transferred)
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

            room.leave(shared_from_this());

            return;
        }
 
        write_msgs.pop_front();

        if (!write_msgs.empty())
            do_write();
    }

    socket_t socket;
    chat_room& room;

    chat_message read_msg;
    chat_message_queue write_msgs;

    uint64_t self = uint64_t(this);
};

class chat_server
{
public:
    chat_server(context_t& ioc, const tcp::endpoint& endpoint) : acceptor(ioc, endpoint)
    {
        do_accept();
    }

    void do_accept()
    {
        acceptor.async_accept(
        [this](std::error_code ec, socket_t socket)
        {
            on_accept(ec, std::move(socket));
        });
    }

    void on_accept(std::error_code ec, socket_t socket)
    {
        if (!ec)
            std::make_shared<chat_session>(std::move(socket), room)->start();
        else
            std::cout << "async_accept " << ec.message() << std::endl;

        do_accept();
    }

private:
    chat_room room;
    tcp::acceptor acceptor;
};

int main(int argc, char* argv[])
{
    try
    {
        if (argc < 3)
        {
            std::cerr << "Usage: " << argv[0] << " <host> <port> [<port> ...]" << std::endl;

            return 1;
        }

        context_t ioc;
        net::inplace_stop_source source;

        std::list<chat_server> servers;
        auto address = net::ip::make_address(argv[1]);

        for (int i = 2; i < argc; ++i)
        {
             tcp::endpoint endpoint(address, std::atoi(argv[i]));
             servers.emplace_back(ioc, endpoint);
        }

        ioc.run(source.get_token());
        source.request_stop();
    }
    catch (std::exception& e)
    {
        std::cerr << "Exception: " << e.what() << std::endl;
    }

    return 0;
}
