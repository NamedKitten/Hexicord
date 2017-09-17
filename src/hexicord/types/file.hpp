#ifndef HEXICORD_TYPES_FILE_HPP
#define HEXICORD_TYPES_FILE_HPP

#include <cstdint>  // uint8_t
#include <string>   // std::string
#include <vector>   // std::vector
#include <iosfwd>   // std::istream

namespace Hexicord {
    /**
     * Struct for (filename, bytes) pair.
     */
    struct File {
        /**
         * Read file specified by `path` to \ref bytes.
         * Last component of path used as filename.
         */
        File(const std::string& path);

        /**
         * Read stream until EOF to \ref bytes.
         */
        File(const std::string& filename, std::istream&& stream);

        /**
         * Use passed vector as file contents.
         */
        File(const std::string& filename, const std::vector<uint8_t>& bytes);

        /**
         * Helper function, write file to std::ofstream(targetPath).
         */
        void write(const std::string& targetPath) const;

        const std::string filename;
        const std::vector<uint8_t> bytes;
    };
} // namespace Hexicord

#endif // HEXICORD_TYPES_FILE_HPP
