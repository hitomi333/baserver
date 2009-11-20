//
// server_work.hpp
// ~~~~~~~~~~~~~~~
//
// Copyright (c) 2009 Xu Ye Jun (moore.xu@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BAS_PROXY_SERVER_WORK_HPP
#define BAS_PROXY_SERVER_WORK_HPP

#include <bas/service_handler.hpp>
#include <bas/client.hpp>

#include <iostream>

#include "client_work.hpp"
#include "client_work_allocator.hpp"

namespace proxy {

class server_work
{
public:
  typedef bas::service_handler<server_work> server_handler_type;
  typedef bas::service_handler<client_work> client_handler_type;
  typedef bas::client<client_work, client_work_allocator> client_type;

  server_work(client_type& client)
    : client_(client),
      child_handler_(0)
  {
  }

/*
  void on_set_parent(server_handler_type& handler, server_handler_type* parent_handler)
  {
  }
*/

  void on_set_child(server_handler_type& handler, client_handler_type* child_handler)
  {
    child_handler_ = child_handler;
  }
  
  void on_clear(server_handler_type& handler)
  {
  }

  void on_open(server_handler_type& handler)
  {
    client_.connect(handler);
  }

  void on_read(server_handler_type& handler, std::size_t bytes_transferred)
  {
    child_handler_->post_parent(bas::event(bas::event::parent_write, bytes_transferred));
  }

  void on_write(server_handler_type& handler, std::size_t bytes_transferred)
  {
    handler.async_read_some();
  }

  void on_close(server_handler_type& handler, const boost::system::error_code& e)
  {
    if (child_handler_ != 0)
    {
      child_handler_->post_parent(bas::event(bas::event::parent_close));
      child_handler_ = 0;
    }

    switch (e.value())
    {
      // Operation successfully completed.
      case 0:
      case boost::asio::error::eof:
        std::cout << "source ok ...............\n";
        std::cout.flush();
        break;

      // Connection breaked.
      case boost::asio::error::connection_aborted:
      case boost::asio::error::connection_reset:
      case boost::asio::error::connection_refused:
        break;

      // Other error.
      case boost::asio::error::timed_out:
      case boost::asio::error::no_buffer_space:
      default:
        std::cout << "source error " << e << " message " << e.message() << "\n";
        std::cout.flush();
        break;
    }
  }

  void on_parent(server_handler_type& handler, const bas::event event)
  {
  }

  void on_child(server_handler_type& handler, const bas::event event)
  {
    switch (event.state_)
    {
      case bas::event::child_open:
        handler.async_read_some();
        break;
      case bas::event::child_write:
        handler.async_write(boost::asio::buffer(child_handler_->read_buffer().data(), event.value_));
        break;
      case bas::event::child_close:
        child_handler_ = 0;
        handler.close();
        break;
    }
  }

private:
  client_type& client_;
  
  client_handler_type* child_handler_;
};

} // namespace proxy

#endif // BAS_PROXY_SERVER_WORK_HPP
