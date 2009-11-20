//
// client_work.hpp
// ~~~~~~~~~~~~~~~
//
// Copyright (c) 2009 Xu Ye Jun (moore.xu@gmail.com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BAS_PROXY_CLIENT_WORK_HPP
#define BAS_PROXY_CLIENT_WORK_HPP

#include <bas/service_handler.hpp>

#include <iostream>

namespace proxy {

class server_work;

class client_work
{
public:
  typedef bas::service_handler<client_work> client_handler_type;
  typedef bas::service_handler<server_work> server_handler_type;

  client_work()
    : parent_handler_(0)
  {
  }
  
  void on_set_parent(client_handler_type& handler, server_handler_type* parent_handler)
  {
    parent_handler_ = parent_handler;
  }

/*
  void on_set_child(client_handler_type& handler, client_handler_type* child_handler)
  {
  }
*/
  void on_clear(client_handler_type& handler)
  {
  }
  
  void on_open(client_handler_type& handler)
  {
    parent_handler_->post_child(bas::event(bas::event::child_open));
  }

  void on_read(client_handler_type& handler, std::size_t bytes_transferred)
  {
    parent_handler_->post_child(bas::event(bas::event::child_write, bytes_transferred));
  }

  void on_write(client_handler_type& handler, std::size_t bytes_transferred)
  {
    handler.async_read_some();
  }

  void on_close(client_handler_type& handler, const boost::system::error_code& e)
  {
    if (parent_handler_ != 0)
    {
      parent_handler_->post_child(bas::event(bas::event::child_close));
      parent_handler_ = 0;
    }

    switch (e.value())
    {
      // Operation successfully completed.
      case 0:
      case boost::asio::error::eof:
        std::cout << "target ok ***************\n";
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
        std::cout << "target error " << e << " message " << e.message() << "\n";
        std::cout.flush();
        break;
    }
  }
  
  void on_parent(client_handler_type& handler, const bas::event event)
  {
    switch (event.state_)
    {
      case bas::event::parent_write:
        handler.async_write(boost::asio::buffer(parent_handler_->read_buffer().data(), event.value_));
        break;

      case bas::event::parent_close:
        parent_handler_ = 0;
        handler.close();
        break;
    }
  }

  void on_child(client_handler_type& handler, const bas::event event)
  {
  }

private:
  server_handler_type* parent_handler_;
};

} // namespace proxy

#endif // BAS_PROXY_CLIENT_WORK_HPP
