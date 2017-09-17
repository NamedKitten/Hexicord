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

#ifndef HEXICORD_WSS_HPP
#define HEXICORD_WSS_HPP

#include <string>        // std::string
#include <vector>        // std::vector
#include <memory>        // std::enable_shared_from_this, std::shared_ptr
#include <mutex>         // std::mutex
#include <functional>    // std::function
#include <unordered_map> // std::unordered_map
namespace boost { namespace asio { class io_service; } namespace system { class error_code; }}
namespace Hexicord { class WSSTLSConnection; }

/**
 *  \file wss.hpp
 *
 *  Better interface for WebSockets on top of low-level boost.beast.
 */

namespace Hexicord {

    /**
     *  High-level beast WebSockets wrapper. Provides basic I/O operations:
     *  read, send, async read, async write.
     */
    class TLSWebSocket : std::enable_shared_from_this<TLSWebSocket> {
    public:
        using AsyncReadCallback = std::function<void(TLSWebSocket&, const std::vector<uint8_t>&, boost::system::error_code)>;
        using AsyncSendCallback = std::function<void(TLSWebSocket&, boost::system::error_code)>;

        /**
         *  Construct unconnected WebSocket. use handshake for connection.
         */
        TLSWebSocket(boost::asio::io_service& ioService);

        /**
         *  Calls shutdown().
         */
        ~TLSWebSocket();

        /**
         *  Send message and block until transmittion finished.
         *
         *  \throws boost::system::system_error on any error.
         *
         *  This method is thread-safe.
         */
        void sendMessage(const std::vector<uint8_t>& message);

        /**
         *  Read message if any, blocks if there is no message.
         *
         *  \throws boost::system::system_error on any error.
         *
         *  This method is thread-safe.
         */
        std::vector<uint8_t> readMessage();

        /**
         *  Asynchronously read message and call callback when done (or error occured).
         *
         *  \warning For now there is no way to cancel this operation.
         *  \warning TLSWebSocket *MUST* be allocated in heap and stored
         *          in std::shared_ptr for this function to work correctly.
         *          Otherwise UB will occur.
         *
         *  This method is thread-safe.
         */
        void asyncReadMessage(AsyncReadCallback callback);

        /**
         *  Asynchronusly send message and call callback when done (or error occured).
         *
         *  \warning For now there is no way to cancel this operation.
         *  \warning TLSWebSocket *MUST* be allocated in heap and stored
         *          in std::shared_ptr for this function to work correctly.
         *          Otherwise UB will occur.
         *
         *  This method is NOT thread-safe.
         */
        void asyncSendMessage(const std::vector<uint8_t>& message, AsyncSendCallback callback);

        /**
         *  Perform TCP handshake, TLS handshake and WS handshake.
         *
         *  \throws boost::system::system_error on any error.
         *
         *  This method is thread-safe.
         */
        void handshake(const std::string& servername, const std::string& path, unsigned short port = 443, const std::unordered_map<std::string, std::string>& additionalHeaders = {});

        /**
         *  Discard any remaining messages until close frame and teardown TCP connection.
         *  Error occured while when closing is ignored. TLSWebSocket instance is no longer
         *  usable after this call.
         *
         *  This method is thread-safe.
         */
        void shutdown();

        bool isSocketOpen() const;

        std::shared_ptr<WSSTLSConnection> connection;
    private:
        const std::string servername;
        std::mutex connectionMutex;
    };
}

#endif // HEXICORD_WSS_HPP
