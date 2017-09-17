#include "hexicord/internal/rest.hpp"

#include <cstdint>                                  // uint8_t
#include <locale>                                   // std::tolower, std::locale
#include <boost/asio/ssl/rfc2818_verification.hpp>  // boost::asio::ssl::rfc2818_verification.hpp
#include <boost/asio/connect.hpp>                   // boost::asio::connect
#include <boost/asio/io_service.hpp>                // boost::asio::io_service
#include <boost/asio/ip/tcp.hpp>                    // boost::asio::ip::tcp
#include <boost/asio/ssl/error.hpp>                 // boost::asio::ssl::error
#include <boost/asio/ssl/context.hpp>               // boost::asio::ssl::context
#include <boost/asio/ssl/stream.hpp>                // boost::asio::ssl::stream
#include <boost/beast/http/error.hpp>               // boost::beast::http::error
#include <boost/beast/http/write.hpp>               // boost::beast::http::write
#include <boost/beast/http/read.hpp>                // boost::beast::http::read
#include <boost/beast/http/vector_body.hpp>         // boost::beast::http::vector_body
#include <boost/beast/core/flat_buffer.hpp>         // boost::beast::flat_buffer
#include "hexicord/internal/utils.hpp"              // Hexicord::Utils::randomAsciiString

namespace ssl = boost::asio::ssl;
using     tcp = boost::asio::ip::tcp;

namespace Hexicord { namespace REST {
struct HTTPSConnectionInternal {
    explicit HTTPSConnectionInternal(boost::asio::io_service& ios)
        : tlsctx(boost::asio::ssl::context::tlsv12_client)
        , stream(ios, tlsctx) {}

    boost::asio::ssl::context tlsctx;
    boost::asio::ssl::stream<boost::asio::ip::tcp::socket> stream;
};

namespace _Detail {
    std::string stringToLower(const std::string& input) {
        std::string result;
        result.reserve(input.size());

        for (char ch : input) {
            result.push_back(std::tolower(ch, std::locale("C")));
        }
        return result;
    }
} // namespace _Detail

HTTPSConnection::HTTPSConnection(boost::asio::io_service& ioService, const std::string& serverName) 
    : serverName(serverName)
    , connection(new HTTPSConnectionInternal(ioService)) {

    connection->tlsctx.set_verify_mode(ssl::verify_peer | ssl::verify_fail_if_no_peer_cert);
    connection->tlsctx.set_verify_callback(ssl::rfc2818_verification(serverName));
    connection->tlsctx.set_default_verify_paths();
}

void HTTPSConnection::open() {
    tcp::resolver resolver(connection->stream.get_io_service());

    boost::asio::connect(connection->stream.next_layer(), resolver.resolve({ serverName, "https" }));
    connection->stream.next_layer().set_option(tcp::no_delay(true));
    connection->stream.handshake(ssl::stream_base::client);
    alive = true;
}

void HTTPSConnection::close() {
    boost::system::error_code ec;
    connection->stream.shutdown(ec);
    if (ec && 
        ec != boost::asio::error::eof && 
        ec != boost::asio::ssl::error::stream_truncated &&
        ec != boost::asio::error::broken_pipe &&
        ec != boost::asio::error::connection_reset) {

        throw boost::system::system_error(ec);
    }
    connection->stream.next_layer().close();
    alive = false;
}

bool HTTPSConnection::isOpen() const {
    return connection->stream.lowest_layer().is_open() && alive;
}

HTTPResponse HTTPSConnection::request(const HTTPRequest& request) {
    //
    // Prepare request
    //
    boost::beast::http::request<boost::beast::http::vector_body<uint8_t> > rawRequest;
    
    rawRequest.method_string(request.method);
    rawRequest.target(request.path);
    rawRequest.version = request.version;

    // Set default headers. 
    rawRequest.set("User-Agent", "Generic HTTP 1.1 Client");
    rawRequest.set("Connection", "keep-alive");
    rawRequest.set("Accept",     "*/*");
    rawRequest.set("Host",       serverName);
    if (!request.body.empty()) {
        rawRequest.set("Content-Length", std::to_string(request.body.size()));
        rawRequest.set("Content-Type",   "application/octet-stream");
    }

    // Set per-connection headers.
    for (const auto& header : connectionHeaders) {
        rawRequest.set(header.first, header.second);
    }

    // Set per-request
    for (const auto& header : request.headers) {
        rawRequest.set(header.first, header.second);
    }

    if (!request.body.empty()) {
        rawRequest.body = request.body;
    }

    //
    // Perform request.
    //
    
    boost::system::error_code ec;

    rawRequest.prepare_payload();
    alive = false;
    boost::beast::http::write(connection->stream, rawRequest, ec);
    if (ec && ec != boost::beast::http::error::end_of_stream) throw boost::system::system_error(ec);

    boost::beast::http::response<boost::beast::http::vector_body<uint8_t> > response;
    boost::beast::flat_buffer buffer;
    boost::beast::http::read(connection->stream, buffer, response);

    REST::HTTPResponse responseStruct;
    responseStruct.statusCode = response.result_int();
    responseStruct.body      = response.body;
    for (const auto& header : response) {
        responseStruct.headers.insert({ header.name_string().to_string(), header.value().to_string() });
    }
    alive = (response["Connection"].to_string() != "close");
    
    return responseStruct;
}

HTTPRequest buildMultipartRequest(const std::vector<MultipartEntity>& elements) {
    HTTPRequest request;
    std::ostringstream oss;

    // XXX: There is some reason for 400 Bad Request coming from Utils::randomAsciiString.
    // std::string boundary = Utils::randomAsciiString(64);
    std::string boundary = "LPN3rnFZYl77S6RI2YHlqA1O1NbvBDelp1lOlMgjSm9VaOV7ufw5fh3qvy2JUq";

    request.headers["Content-Type"] = std::string("multipart/form-data; boundary=") + boundary;

    for (auto it = elements.begin(); it != elements.end(); ++it) {
        const auto& element = *it;

        oss << "--" << boundary << "\r\n"
            << "Content-Disposition: form-data; name=\"" << element.name << "\"";
        if (!element.filename.empty()) {
            oss << "; filename=\"" << element.filename << '"';
        }
        oss << "\r\n";
        for (const auto& header : element.additionalHeaders) {
            oss << header.first << ": " << header.second << "\r\n";
        }
        oss << "\r\n";
        for (uint8_t byte : element.body) {
            oss << byte;
        }
        oss << "\r\n";

        oss << "--" << boundary;
        if (it == elements.end() - 1) {
            oss << "--";
        }
        oss << "\r\n";
    }

    std::string resultStr = oss.str();
    request.body = std::vector<uint8_t>(resultStr.begin(), resultStr.end());
    return request;
}

}} // namespace Hexicord::REST
