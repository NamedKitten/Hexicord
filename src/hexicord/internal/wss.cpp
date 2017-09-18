// Hexicord - Discord API library for C++11 using boost libraries.
// Copyright © 2017 Maks Mazurov (fox.cpp) <foxcpp@yandex.ru>
// 
// Permission is hereby granted, free of charge, to any person obtaining
// a copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
// OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
// IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
// DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
// TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE
// OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

#include "hexicord/internal/wss.hpp"
#include <boost/beast/core/flat_buffer.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ssl/context.hpp>         // boost::asio::ssl::context
#include <boost/asio/ssl/stream.hpp>          // boost::asio::ssl::stream
#include <boost/beast/websocket/stream.hpp>   // websocket::stream
#include <boost/beast/websocket/ssl.hpp>
#include <boost/asio/ip/tcp.hpp>              // tcp::socket

namespace websocket = boost::beast::websocket;
using tlsstream     = boost::asio::ssl::stream<boost::asio::ip::tcp::socket>;
using wssstream     = boost::beast::websocket::stream<tlsstream>;
using ioservice     = boost::asio::io_service;
using tcp           = boost::asio::ip::tcp;
namespace ssl = boost::asio::ssl;

namespace Hexicord {
    struct WSSTLSConnection {
        explicit WSSTLSConnection(boost::asio::io_service& ios)
            : tlsContext(boost::asio::ssl::context::tlsv12_client)
            , wsStream(ios, tlsContext) {}

        boost::asio::ssl::context tlsContext;
        wssstream wsStream;
    };

    TLSWebSocket::TLSWebSocket(boost::asio::io_service& ioService)
        : connection(new WSSTLSConnection(ioService)) {

        connection->tlsContext.set_default_verify_paths();
        connection->tlsContext.set_verify_mode(ssl::verify_peer | ssl::verify_fail_if_no_peer_cert);
    }

    TLSWebSocket::~TLSWebSocket() {
        try {
            if (connection->wsStream.lowest_layer().is_open()) this->shutdown();
        } catch (...) {
            // it's a destructor, we should not allow any exceptions.
        }
    }
    
    void TLSWebSocket::sendMessage(const std::vector<uint8_t>& message) {
        std::lock_guard<std::mutex> lock(connectionMutex);
        connection->wsStream.write(boost::asio::buffer(message.data(), message.size()));
    }
    
    std::vector<uint8_t> TLSWebSocket::readMessage() {
        std::lock_guard<std::mutex> lock(connectionMutex);
        boost::beast::flat_buffer buffer;
    
        connection->wsStream.read(buffer);
    
        auto bufferData = boost::asio::buffer_cast<const uint8_t*>(*buffer.data().begin());
        auto bufferSize = boost::asio::buffer_size(*buffer.data().begin());

        return std::vector<uint8_t>(bufferData, bufferData + bufferSize);
    }

    void TLSWebSocket::asyncReadMessage(const TLSWebSocket::AsyncReadCallback& callback) {
        std::shared_ptr<boost::beast::flat_buffer> buffer(new boost::beast::flat_buffer);

        connection->wsStream.async_read(*buffer, [this, buffer, callback](boost::system::error_code ec, unsigned long length) {
            // buffer captured by value into lambda.
            // so they will exist here and hold ownership.

            auto bufferData = boost::asio::buffer_cast<const uint8_t*>(*buffer->data().begin());
            std::vector<uint8_t> vectorBuffer(bufferData, bufferData + length);

            callback(*this, vectorBuffer, ec);

            // however, buffer ownership will be released here and it will
            // removed. 
        });
    }

    void TLSWebSocket::asyncSendMessage(const std::vector<uint8_t>& message, const TLSWebSocket::AsyncSendCallback& callback) {
        connection->wsStream.async_write(boost::asio::buffer(message.data(), message.size()), [this, callback] (boost::system::error_code ec) {
            callback(*this, ec);
        });
    }

    void TLSWebSocket::handshake(const std::string& servername, const std::string& path, unsigned short port, const std::unordered_map<std::string, std::string>& additionalHeaders) {
        std::lock_guard<std::mutex> lock(connectionMutex);

        tcp::resolver resolver(connection->wsStream.get_io_service());

        boost::asio::connect(connection->wsStream.lowest_layer(), resolver.resolve({ servername, std::to_string(port) }));
        connection->wsStream.next_layer().handshake(ssl::stream_base::client);
        connection->wsStream.handshake_ex(servername, path, [&additionalHeaders](websocket::request_type& request) {
            for (const auto& header : additionalHeaders) {
                request.set(header.first, header.second);
            }
        });
    }

    void TLSWebSocket::shutdown() {
        boost::system::error_code ec;
        connection->wsStream.close(websocket::close_code::normal, ec);
        if (ec &&
            ec != boost::asio::ssl::error::stream_truncated &&
            ec != boost::asio::error::broken_pipe &&
            ec != boost::asio::error::connection_reset &&
            ec != boost::asio::error::eof) {

            throw boost::system::system_error(ec);
        }

        connection->wsStream.next_layer().shutdown(/* ignored */ ec);
        connection->wsStream.next_layer().next_layer().close();
    }

    bool TLSWebSocket::isSocketOpen() const {
        return connection->wsStream.lowest_layer().is_open();
    }
} // namespace Hexicord
