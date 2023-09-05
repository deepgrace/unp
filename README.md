# unp [![LICENSE](https://img.shields.io/github/license/deepgrace/unp.svg)](https://github.com/deepgrace/unp/blob/master/LICENSE_1_0.txt) [![Language](https://img.shields.io/badge/language-C%2B%2B20-blue.svg)](https://en.cppreference.com/w/cpp/compiler_support) [![Platform](https://img.shields.io/badge/platform-Linux-lightgrey.svg)](https://github.com/deepgrace/unp)

> **Uring based Network Programming**

## Overview
async timer:
```cpp
#include <thread>
#include <iostream>
#include <unp.hpp>

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
```

async file copy:
```cpp
#include <array>
#include <iostream>
#include <filesystem>
#include <unp.hpp>

namespace net = unp;
namespace fs = std::filesystem;

using file = net::stream_file;
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
        net::async_read_some(from, net::buffer(buff),
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
                std::cerr << "async_read " << ec.message() << std::endl;

            source.request_stop();

            return;
        }

        do_write(bytes_transferred);
    }

    void do_write(std::size_t length)
    {
        net::async_write(to, net::buffer(buff.data(), length), 
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

        do_read();
    }

private:
    file from;
    file to;

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
```

## Introduction
unp is a network programming library, which is header-only, extensible and modern C++ oriented.  
It's built on top off the io_uring framework, it's based on the **Proactor** design pattern with performance in mind.  
unp enables you to do network programming with tcp and udp protocols in a straightforward, asynchronous and OOP manner.  

unp provides the following capabilities:
- **tcp**
- **udp**
- **steady_timer**
- **stream_file**
- **random_access_file**

## Compiler requirements
The library relies on a C++20 compiler and standard library, but nothing else is required.

More specifically, unp requires a compiler/standard library supporting the following C++20 features (non-exhaustively):
- concepts
- lambda templates
- All the C++20 type traits from the <type_traits> header

## Building
unp is header-only. To use it just add the necessary `#include` line to your source files, like this:
```cpp
#include <unp.hpp>
```

To build the example with cmake, `cd` to the root of the project and setup the build directory:
```bash
mkdir build
cd build
cmake ..
```

Make and install the executables:
```
make -j4
make install
```

The executables are now located at the `bin` directory of the root of the project.  
The example can also be built with the script `build.sh`, just run it, the executables will be put at the `/tmp` directory.

## Full example
Please see [example](example).

## License
unp is licensed as [Boost Software License 1.0](LICENSE_1_0.txt).
