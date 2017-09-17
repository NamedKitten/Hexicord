#include "hexicord/types/file.hpp"

#include <iterator>                    // std::istreambuf_iterator
#include <algorithm>                   // std::copy
#include <fstream>                     // std::ifstream
#include "hexicord/internal/utils.hpp" // Utils::split

#if _WIN32
    #define PATH_DELIMITER '\\'
#elif !defined(_WIN32) && (defined(__unix__) || defined(__unix) || (defined(__APPLE__) && defined(__MACH__)))
    #define PATH_DELIMITER '/'
#endif

namespace Hexicord {

File::File(const std::string& path)
    : filename(Utils::split(path, PATH_DELIMITER).back())
    , bytes(std::istreambuf_iterator<char>(std::ifstream(path, std::ios_base::binary).rdbuf()),
            std::istreambuf_iterator<char>()) {}

File::File(const std::string& filename, std::istream&& stream)
    : filename(filename)
    , bytes(std::istreambuf_iterator<char>(stream.rdbuf()),
            std::istreambuf_iterator<char>()) {}

File::File(const std::string& filename, const std::vector<uint8_t>& bytes)
    : filename(filename)
    , bytes(bytes) {}

void File::write(const std::string& targetPath) const {
    std::ofstream output(targetPath, std::ios_base::binary | std::ios_base::trunc);
    std::copy(bytes.begin(), bytes.end(), std::ostreambuf_iterator<char>(output.rdbuf()));
}

} // namespace Hexicord
