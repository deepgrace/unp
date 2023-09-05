//
// Copyright (c) 2023-present DeepGrace (complex dot invoke at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/deepgrace/unp
//

#include <array>
#include <iostream>
#include <filesystem>
#include <unp.hpp>

// g++ -std=c++23 -Wall -O3 -Os -s -I example -I include example/random_file_copy.cpp -o /tmp/random_file_copy

namespace net = unp;
namespace fs = std::filesystem;

using file = net::random_access_file;
using context_t = net::io_uring_context;

net::inplace_stop_source source;

class file_copier
{
public:
    file_copier(context_t& ioc, const std::string& from, const std::string& to) :
    from(ioc, from, file::read_only), to(ioc, to, file::write_only | file::create | file::truncate)
    {
        fs::file_status s = fs::status(from);
        fs::permissions(to, s.permissions());
    }

    void start()
    {
        do_read();
    }

    void do_read()
    {
        net::async_read_some_at(from, offset, net::buffer(buff),
        [this](std::error_code ec, std::size_t bytes_transferred)
        {
            on_read(ec, bytes_transferred);
        });
    }

    void on_read(std::error_code ec, std::size_t bytes_transferred)
    {
        if (ec)
        {
            if (ec != std::errc::no_message)
                std::cout << "async_read " << ec.message() << std::endl;

            source.request_stop();

            return;
        }

        do_write(bytes_transferred);
    }

    void do_write(std::size_t length)
    {
        net::async_write_some_at(to, offset, net::buffer(buff.data(), length), 
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

            source.request_stop();

            return;
        }

        offset += bytes_transferred;
        do_read();
    }

private:
    file from;
    file to;

    int64_t offset = 0;
    std::array<char, 4096> buff;
};

int main(int argc, char* argv[])
{
    try
    {
        if (argc != 3)
        {
            std::cerr << "Usage: " << argv[0] << " <from> <to>" << std::endl;

            return 1;
        }

        context_t ioc;
        file_copier copier(ioc, argv[1], argv[2]);

        copier.start();
        ioc.run(source.get_token());
    }
    catch (std::exception& e)
    {
        std::cerr << "Exception: " << e.what() << std::endl;
    }

    return 0;
}
