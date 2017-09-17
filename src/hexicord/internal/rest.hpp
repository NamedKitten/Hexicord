#ifndef HEXICORD_REST_HPP
#define HEXICORD_REST_HPP

#include <cstdint>        // uint8_t
#include <string>         // std::string
#include <vector>         // std::vector
#include <unordered_map>  // std::unordered_map
#include <memory>         // std::shared_ptr
namespace boost { namespace asio { class io_service; }}

namespace Hexicord { namespace REST {
    struct HTTPSConnectionInternal;

    namespace _detail {
        std::string stringToLower(const std::string& input);

        struct CaseInsensibleStringEqual {
            inline bool operator()(const std::string& lhs, const std::string& rhs) const {
                return stringToLower(lhs) == stringToLower(rhs);
            }
        };
    }

    /// Hash-map with case-insensible string keys.
    using HeadersMap = std::unordered_map<std::string, std::string, std::hash<std::string>, _detail::CaseInsensibleStringEqual>;

    struct HTTPResponse {
        unsigned statusCode;
        
        HeadersMap headers;
        std::vector<uint8_t> body;
    };

    struct HTTPRequest {
        std::string method;
        std::string path;
        
        unsigned version;
        std::vector<uint8_t> body;
        HeadersMap headers;
    };

    class HTTPSConnection {
    public:
        HTTPSConnection(boost::asio::io_service& ioService, const std::string& serverName);

        void open();
        void close();

        bool isOpen() const;

        HTTPResponse request(const HTTPRequest& request);

        HeadersMap connectionHeaders;
        const std::string serverName;

    private:
        std::shared_ptr<HTTPSConnectionInternal> connection;

        bool alive = false;
    };

    struct MultipartEntity {
        std::string name;
        std::string filename;
        HeadersMap additionalHeaders;

        std::vector<uint8_t> body;
    };

    HTTPRequest buildMultipartRequest(const std::vector<MultipartEntity>& elements);

}} // namespace Hexicord::REST

#endif // HEXICORD_REST_HPP 
