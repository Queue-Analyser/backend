#ifndef QUEUE_ANALYSER_SERVER_SESSION_H_
#define QUEUE_ANALYSER_SERVER_SESSION_H_
#include "handler.hpp"
#include "error.hpp"

#include <iostream>
#include <memory>
#include <boost/beast.hpp>
#include <boost/asio.hpp>

// Handles an HTTP server connection
class Session : public std::enable_shared_from_this<Session>
{
    // This is the C++11 equivalent of a generic lambda.
    // The function object is used to send an HTTP message.
    struct send_lambda {
        Session& self_;

        explicit send_lambda(Session& self)
            : self_(self)
        {}

        template<bool isRequest, class Body, class Fields>
        void
        operator()(boost::beast::http::message<isRequest, Body, Fields>&& msg) const
        {
            // The lifetime of the message has to extend
            // for the duration of the async operation so
            // we use a shared_ptr to manage it.
            auto sp = std::make_shared<
                boost::beast::http::message<isRequest, Body, Fields>>(std::move(msg));

            // Store a type-erased version of the shared
            // pointer in the class to keep it alive.
            self_.res_ = sp;

            // Write the response
            boost::beast::http::async_write(
                self_.stream_,
                *sp,
                boost::beast::bind_front_handler(
                    &Session::on_write,
                    self_.shared_from_this(),
                    sp->need_eof()));
        }
    };

    boost::beast::tcp_stream stream_;
    boost::beast::flat_buffer buffer_;
    std::shared_ptr<std::string const> doc_root_;
    boost::beast::http::request<boost::beast::http::string_body> req_;
    std::shared_ptr<void> res_;
    send_lambda lambda_;

public:
    // Take ownership of the stream
    Session(
        boost::asio::ip::tcp::socket&& socket,
        std::shared_ptr<std::string const> const& doc_root)
        : stream_(std::move(socket))
        , doc_root_(doc_root)
        , lambda_(*this) {}

    // Start the asynchronous operation
    void run() {
        do_read();
    }


    void do_read();

    void on_read(
        boost::beast::error_code ec,
        std::size_t bytes_transferred);

    void on_write(
        bool close,
        boost::beast::error_code ec,
        std::size_t bytes_transferred);

    void do_close();
};

#endif // QUEUE_ANALYSER_SERVER_SESSION_H_