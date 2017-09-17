#include "hexicord/types/image.hpp"

#include <boost/asio/io_service.hpp>   // boost::asio::io_service
#include "hexicord/internal/utils.hpp" // Hexicord::Utils::Magic, Hexicord::Utils::base64Encode
#include "hexicord/exceptions.hpp"     // Hexicord::LogicError
#include "hexicord/internal/rest.hpp"  // Hexicord::REST

namespace Hexicord {

Image::Image(const File& file, ImageFormat format)
    : format(format == ImageFormat::Detect ? detectFormat(file) : format)
    , file(file)
{}

std::string Image::toAvatarData() const {
    std::string mimeType;
    if (format == Jpeg) mimeType = "image/jpeg";
    if (format == Png)  mimeType = "image/png";
    if (format == Webp) mimeType = "image/webp";
    if (format == Gif)  mimeType = "image/gif";

    return std::string("data:") + mimeType + ";base64," + Utils::base64Encode(file.bytes);
}

ImageFormat Image::detectFormat(const File& file) {
    if (Utils::Magic::isJfif(file.bytes)) return ImageFormat::Jpeg;
    if (Utils::Magic::isPng(file.bytes))  return ImageFormat::Png;
    if (Utils::Magic::isWebp(file.bytes)) return ImageFormat::Webp;
    if (Utils::Magic::isGif(file.bytes))  return ImageFormat::Gif;

    throw LogicError("Failed to detect image format.", -1);
}

namespace _Detail {
    std::vector<uint8_t> cdnDownload(boost::asio::io_service& ioService, const std::string& path) {
        REST::HTTPSConnection connection(ioService, "cdn.discordapp.com");
        connection.open();

        REST::HTTPRequest request;
        request.method  = "GET";
        request.path    = path;
        request.version = 11;

        REST::HTTPResponse response = connection.request(request);
        if (response.body.empty()) {
            throw LogicError("Response body is empty (are you trying to download non-animated avatar as GIF?)", -1);
        }
        if (response.statusCode != 200) {
            throw LogicError(std::string("HTTP status code: ") + std::to_string(response.statusCode), -1);
        }

        return response.body;
    }
} // namespace _Detail

} // namespace Hexicord
