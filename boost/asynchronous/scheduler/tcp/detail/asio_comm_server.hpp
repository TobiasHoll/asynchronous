// Boost.Asynchronous library
//  Copyright (C) Christophe Henry 2013
//
//  Use, modification and distribution is subject to the Boost
//  Software License, Version 1.0.  (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// For more information, see http://www.boost.org

#ifndef BOOST_ASYNCHRONOUS_SCHEDULER_TCP_ASIO_COMM_SERVER_HPP
#define BOOST_ASYNCHRONOUS_SCHEDULER_TCP_ASIO_COMM_SERVER_HPP

#include <functional>
#include <string>

#include <boost/asio.hpp>
#include <boost/shared_ptr.hpp>

#include <boost/asynchronous/trackable_servant.hpp>
#include <boost/asynchronous/scheduler/tcp/detail/server_connection.hpp>
#include <boost/asynchronous/extensions/asio/tss_asio.hpp>
#include <boost/asynchronous/servant_proxy.h>

namespace boost { namespace asynchronous { namespace tcp {

struct asio_comm_server : boost::asynchronous::trackable_servant<>
{
    asio_comm_server(boost::asynchronous::any_weak_scheduler<> scheduler,
                     std::string const & address,
                     unsigned int port,
                     std::function<void(boost::shared_ptr<boost::asynchronous::tcp::server_connection>)> connectionHandler)
    : boost::asynchronous::trackable_servant<>(scheduler)
        , m_connection_handler(connectionHandler)
        , m_acceptor(*boost::asynchronous::get_io_service<>())
        , m_socket(*boost::asynchronous::get_io_service<>())
    {
        // Open the acceptor with the option to reuse the address (i.e. SO_REUSEADDR)
        boost::asio::ip::tcp::resolver resolver(*boost::asynchronous::get_io_service<>());
        boost::asio::ip::tcp::endpoint endpoint = *resolver.resolve({address, to_string(port)});
        m_acceptor.open(endpoint.protocol());
        m_acceptor.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
        m_acceptor.bind(endpoint);
        m_acceptor.listen();

        do_accept();
    }

private:
    /// Perform an asynchronous accept operation
    void do_accept()
    {
        m_acceptor.async_accept(m_socket,
            [this](boost::system::error_code ec)
            {
                // Check whether the server was stopped by a signal before this completion handler had
                // a chance to run.
                if (!m_acceptor.is_open())
                {
                    return;
                }

                if (!ec)
                {
                    const auto connection = boost::make_shared<boost::asynchronous::tcp::server_connection>(std::move(m_socket));
                    m_connection_handler(connection);
                }

                do_accept();
            });
    }

private:
    std::function<void(boost::shared_ptr<boost::asynchronous::tcp::server_connection>)> m_connection_handler;
    boost::asio::ip::tcp::acceptor m_acceptor;
    boost::asio::ip::tcp::socket m_socket;
};

class asio_comm_server_proxy : public boost::asynchronous::servant_proxy<asio_comm_server_proxy, boost::asynchronous::tcp::asio_comm_server>
{
public:
    template<class Scheduler>
    asio_comm_server_proxy( Scheduler scheduler,
                            std::string const & address,
                            unsigned int port,
                            std::function<void(boost::shared_ptr<boost::asynchronous::tcp::server_connection>)> connectionHandler)
        : boost::asynchronous::servant_proxy<asio_comm_server_proxy, boost::asynchronous::tcp::asio_comm_server>(scheduler,
                                                                                                                 address,
                                                                                                                 port,
                                                                                                                 connectionHandler)
    {}
};

}}}
#endif // BOOST_ASYNCHRONOUS_SCHEDULER_TCP_ASIO_COMM_SERVER_HPP