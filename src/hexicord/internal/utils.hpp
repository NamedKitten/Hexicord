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

#ifndef HEXICORD_UTILS_HPP
#define HEXICORD_UTILS_HPP

#include <cstdint>        // uint8_t
#include <vector>         // std::vector
#include <string>         // std::string
#include <unordered_map>  // std::unordered_map

/**
 *  Reusable code snippets.
 */

namespace Hexicord { namespace Utils {
    /**
     *  Fast but in-percise type identification based on first ("magic") bytes.
     */
    namespace Magic {
        bool isGif(const std::vector<uint8_t>& bytes);
        bool isJfif(const std::vector<uint8_t>& bytes);
        bool isPng(const std::vector<uint8_t>& bytes);
        bool isWebp(const std::vector<uint8_t>& bytes);
    }

    /**
     *  scheme://domain/otherstuff?aas=b#as -> domain
     */
    std::string domainFromUrl(const std::string& url);

    /**
     *  Encode arbitrary data using base64.
     */
    std::string base64Encode(const std::vector<uint8_t>& bytes);

    std::string urlEncode(const std::string& raw);
    std::string makeQueryString(const std::unordered_map<std::string, std::string>& queryVariables);

    std::vector<std::string> split(const std::string& str, char delimiter);

    bool isNumber(const std::string& input);

    /**
     * Extract part of URL that have "per-route" ratelimit.
     */
    std::string getRatelimitDomain(const std::string& path);

    // Construct it somewhere to make sure PRNG is initialized.
    struct RandomSeedGuard { RandomSeedGuard(); };

    std::string randomAsciiString(unsigned length);
}} // namespace Hexicord::Utils

#endif // HEXICORD_UTILS_HPP
