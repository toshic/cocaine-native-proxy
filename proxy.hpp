/*
    Copyright (c) 2013 Andrey Goryachev <andrey.goryachev@gmail.com>
    Copyright (c) 2011-2013 Other contributors as noted in the AUTHORS file.

    This file is part of Cocaine.

    Cocaine is free software; you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    Cocaine is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef COCAINE_NATIVE_PROXY_HPP
#define COCAINE_NATIVE_PROXY_HPP

#include "service_pool.hpp"
#include <cocaine/framework/services/app.hpp>
#include <thevoid/server.hpp>

namespace cocaine { namespace proxy {

class proxy :
    public ioremap::thevoid::server<proxy>
{
public:
    struct on_enqueue;

    struct response_stream :
        public cocaine::framework::basic_stream<std::string>,
        public std::enable_shared_from_this<response_stream>
    {
        response_stream(const std::shared_ptr<on_enqueue>& req);

        virtual
        void
        write(std::string&& chunk);

        virtual
        void
        error(const std::exception_ptr& e) {
            std::unique_lock<std::mutex> guard(m_access_mutex);
            error(e, guard);
        }

        virtual
        void
        close() {
            std::unique_lock<std::mutex> guard(m_access_mutex);
            close(boost::system::error_code(), guard);
        }

        virtual
        bool
        closed() const {
            return m_closed;
        }

    private:
        void
        write_headers(std::string&& packed, const std::unique_lock<std::mutex>&);

        void
        write_body(std::string&& packed, const std::unique_lock<std::mutex>&);

        void
        error(const std::exception_ptr& e, const std::unique_lock<std::mutex>&);

        void
        close(const boost::system::error_code& ec, const std::unique_lock<std::mutex>&);

        void
        on_error(const boost::system::error_code &err);

    private:
        std::weak_ptr<cocaine::framework::logger_t> m_logger;
        std::shared_ptr<on_enqueue> m_request;

        bool m_chunked;
        size_t m_content_length;
        bool m_body;

        std::atomic<bool> m_closed;

        std::mutex m_access_mutex;
    };
    friend struct response_stream;

    struct on_enqueue :
        public ioremap::thevoid::simple_request_stream<proxy>,
        public std::enable_shared_from_this<on_enqueue>
    {
        friend struct response_stream;

        virtual
        void
        on_request(const ioremap::thevoid::http_request &req,
                   const boost::asio::const_buffer &buffer);

        const std::string&
        app() const {
            return m_application;
        }

        const std::string&
        event() const {
            return m_event;
        }

    private:
        std::string m_application;
        std::string m_event;
    };
    friend struct on_enqueue;

    struct on_ping :
        public ioremap::thevoid::simple_request_stream<proxy>
    {
        virtual
        void
        on_request(const ioremap::thevoid::http_request &req,
                   const boost::asio::const_buffer &buffer);
    };

public:
    bool
    initialize(const rapidjson::Value &config);

    ~proxy();

    std::map<std::string, std::string>
    get_statistics() const;

private:
    typedef std::map<std::string, std::shared_ptr<service_pool<cocaine::framework::app_service_t>>>
            clients_map_t;

    size_t m_pool_size;
    unsigned int m_reconnect_timeout;
    unsigned int m_request_timeout;

    std::shared_ptr<cocaine::framework::service_manager_t> m_service_manager;
    clients_map_t m_services;
    mutable std::mutex m_services_mutex;
};

}} // namespace cocaine::proxy

#endif // COCAINE_NATIVE_PROXY_HPP

