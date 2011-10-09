//
// posix_main_server.cpp
// ~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2008 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#define BOOST_LIB_DIAGNOSTIC

#include <iostream>
#include <string>
#include <boost/thread.hpp>
#include <boost/bind.hpp>
#include <boost/lexical_cast.hpp>

#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <bas/server.hpp>

#include "ssl_server_work.hpp"
#include "ssl_server_work_allocator.hpp"

#if !defined(_WIN32)

#include <pthread.h>
#include <signal.h>

int main(int argc, char* argv[])
{
  try
  {
    // Check command line arguments.
    if (argc != 9)
    {
      std::cerr << "Usage: ssl_server <address> <port> <io_pool_size> <work_pool_init_size> <work_pool_high_watermark> <preallocated_handler_number> <data_buffer_size> <timeout_seconds>\n";
      std::cerr << "  For IPv4, try:\n";
      std::cerr << "    ssl_server 0.0.0.0 1000 4 4 16 500 256 0\n";
      std::cerr << "  For IPv6, try:\n";
      std::cerr << "    ssl_server 0::0 1000 4 4 16 500 256 0\n";
      return 1;
    }

    // Initialise server.
    unsigned short port = boost::lexical_cast<unsigned short>(argv[2]);
    std::size_t io_pool_size = boost::lexical_cast<std::size_t>(argv[3]);
    std::size_t work_pool_init_size = boost::lexical_cast<std::size_t>(argv[4]);
    std::size_t work_pool_high_watermark = boost::lexical_cast<std::size_t>(argv[5]);
    std::size_t preallocated_handler_number = boost::lexical_cast<std::size_t>(argv[6]);
    std::size_t read_buffer_size = boost::lexical_cast<std::size_t>(argv[7]);
    std::size_t timeout_seconds = boost::lexical_cast<std::size_t>(argv[8]);

    typedef boost::asio::ssl::stream<boost::asio::ip::tcp::socket> ssl_socket;
    typedef bas::server<echo::ssl_server_work, echo::ssl_server_work_allocator, ssl_socket> server;
    typedef bas::service_handler_pool<echo::ssl_server_work, echo::ssl_server_work_allocator, ssl_socket> server_handler_pool;

    server s(argv[1],
        port,
        io_pool_size,
        work_pool_init_size,
        work_pool_high_watermark,
        new server_handler_pool(new echo::ssl_server_work_allocator(),
            preallocated_handler_number,
            read_buffer_size,
            0,
            timeout_seconds));

    // Block all signals for background thread.
    sigset_t new_mask;
    sigfillset(&new_mask);
    sigset_t old_mask;
    pthread_sigmask(SIG_BLOCK, &new_mask, &old_mask);

    // Run server in background thread.
    boost::thread t(boost::bind(&server::run, &s));

    // Restore previous signals.
    pthread_sigmask(SIG_SETMASK, &old_mask, 0);

    // Wait for signal indicating time to shut down.
    sigset_t wait_mask;
    sigemptyset(&wait_mask);
    sigaddset(&wait_mask, SIGINT);
    sigaddset(&wait_mask, SIGQUIT);
    sigaddset(&wait_mask, SIGTERM);
    pthread_sigmask(SIG_BLOCK, &wait_mask, 0);
    int sig = 0;
    sigwait(&wait_mask, &sig);

    // Stop the server.
    s.stop();
    t.join();
  }
  catch (std::exception& e)
  {
    std::cerr << "exception: " << e.what() << "\n";
  }

  return 0;
}

#endif // !defined(_WIN32)
