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

#include "hexicord/internal/utils.hpp"

#include <cctype>       // std::isalnum
#include <ctime>        // std::time
#include <cstdlib>      // std::rand, std::srand, size_t
#include <cassert>      // assert
#include <stdexcept>    // std::invalid_argument
#include <sstream>      // std::ostringstream
#include <iomanip>      // std::setw
#include <stdexcept>    // std::invalid_argument
#include <cstdlib>      // std::rand, std::rand

namespace Hexicord { namespace Utils {
    namespace Magic {
        bool isGif(const std::vector<uint8_t>& bytes) {
            // according to http://fileformats.archiveteam.org/wiki/GIF
            return bytes.size() >= 6 &&  // TODO: check against minimal headers size
                bytes[0] == 'G' &&  // should begin with 'GIF'
                bytes[1] == 'I' &&
                bytes[2] == 'F' &&
                ( // then GIF version, '87a' or '89a'
                    (
                        bytes[3] == '8' &&
                        bytes[4] == '7' &&
                        bytes[5] == 'a'
                    ) || (
                        bytes[3] == '8' &&
                        bytes[4] == '9' &&
                        bytes[5] == 'a'
                    )
                ); 
        }

        bool isJfif(const std::vector<uint8_t>& bytes) {
            // according to http://fileformats.archiveteam.org/wiki/JFIF
            return bytes.size() >= 3 && // TODO: check against minimal headers size
                bytes[0] == 0xFF &&
                bytes[1] == 0xD8 &&
                bytes[2] == 0xFF;
        }

        bool isPng(const std::vector<uint8_t>& bytes) {
            // according to https://www.w3.org/TR/PNG/#5PNG-file-signature
            return bytes.size() >= 12 && // signature + single no-data chunk size
                bytes[0] == 137 &&
                bytes[1] == 'P' &&
                bytes[2] == 'N' &&
                bytes[3] == 'G' &&
                bytes[4] == 13  && // CR
                bytes[5] == 10  && // LF
                bytes[6] == 26  && // SUB
                bytes[7] == 10;    // LF
        }

        bool isWebp(const std::vector<uint8_t>& bytes) {
            // according to https://developers.google.com/speed/webp/docs/riff_container?csw=1
            return bytes.size() >= 12 && // 'RIFF' + size + 'WEBP'
                bytes[0]  == 'R' &&
                bytes[1]  == 'I' &&
                bytes[2]  == 'F' &&
                bytes[3]  == 'F' &&
                bytes[8]  == 'W' &&
                bytes[9]  == 'E' &&
                bytes[10] == 'B' &&
                bytes[11] == 'P';
        }
    }

	std::string domainFromUrl(const std::string& url) {
	   enum State {
	       ReadingSchema,
	       ReadingSchema_Colon,
	       ReadingSchema_FirstSlash,
	       ReadingSchema_SecondSlash,
	       ReadingDomain,
	       End 
	   } state = ReadingSchema;

	   std::string result;
	   for (char ch : url) {
	       switch (state) {
	       case ReadingSchema:
	           if (ch == ':') {
	               state = ReadingSchema_Colon;
	           } else if (!std::isalnum(ch)) {
	               throw std::invalid_argument("Missing colon after schema.");
	           }
	           break;
	       case ReadingSchema_Colon:
	           if (ch == '/') {
	               state = ReadingSchema_FirstSlash;
	           } else {
	               throw std::invalid_argument("Missing slash after schema.");
	           }
	           break;
	       case ReadingSchema_FirstSlash:
	           if (ch == '/') {
	               state = ReadingSchema_SecondSlash;
	           } else {
	               throw std::invalid_argument("Missing slash after schema.");
	           }
	           break;
	       case ReadingSchema_SecondSlash:
	           if (std::isalnum(ch) || ch == '-' || ch == '_') {
	               state = ReadingDomain;
	               result += ch;
	           } else {
	               throw std::invalid_argument("Invalid first domain character.");
	           }
	           break;
	       case ReadingDomain:
	           if (ch == '/') {
	               state = End;
	           } else {
	               result += ch;
	           }
	       case End:
	           break;
	       }
	       if (state == End) break;
	   }
	   return result;
	}

	std::string base64Encode(const std::vector<uint8_t>& data)
	{
	   static constexpr uint8_t base64Map[65] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

	   // Use = signs so the end is properly padded.
	   std::string result((((data.size() + 2) / 3) * 4), '=');
	   size_t outpos = 0;
	   int bits_collected = 0;
	   unsigned accumulator = 0;

	   for (uint8_t byte : data) {
	      accumulator = (accumulator << 8) | (byte & 0xffu);
	      bits_collected += 8;
	      while (bits_collected >= 6) {
	         bits_collected -= 6;
	         result[outpos++] = base64Map[(accumulator >> bits_collected) & 0x3fu];
	      }
	   }
	   if (bits_collected > 0) { // Any trailing bits that are missing.
	      assert(bits_collected < 6);
	      accumulator <<= 6 - bits_collected;
	      result[outpos++] = base64Map[accumulator & 0x3fu];
	   }
	   assert(outpos >= (result.size() - 2));
	   assert(outpos <= result.size());
	   return result;
	}

    std::string urlEncode(const std::string& raw) {
        std::ostringstream resultStream;

        resultStream.fill('0');
        resultStream << std::hex;
        
        for (char ch : raw) {
            if (std::isalnum(ch) || ch == '.' || ch == '~' || ch == '_' || ch == '-') {
                resultStream << ch;
            } else {
                resultStream << std::uppercase;
                resultStream << '%' << std::setw(2) << int(ch);
                resultStream << std::nouppercase;
            }
        }
        return resultStream.str();
    }

    std::string makeQueryString(const std::unordered_map<std::string, std::string>& queryVariables) {
        std::string queryString;
        if (!queryVariables.empty()) {
            queryString += '?';
            for (auto queryParam : queryVariables) {
                queryString += queryParam.first;
                queryString += '=';
                queryString += Utils::urlEncode(queryParam.second);
                queryString += '&';
            }
            queryString.pop_back(); // remove extra & at end.
        }
        return queryString;
    }

    std::vector<std::string> split(const std::string& str, char delimiter) {
        std::istringstream iss(str);
        std::vector<std::string> result;

        std::string buffer;
        while (std::getline(iss, buffer, delimiter)) {
            if (!buffer.empty()) {
                result.push_back(buffer);
            }
        }
        return result;
    }

    bool isNumber(const std::string& input) {
        for (char ch : input) {
            if (!::isdigit(ch)) return false;
        }
        return true;
    }

    std::string getRatelimitDomain(const std::string& path) {
        std::vector<std::string> splittenPath = split(path, '/');

        std::string ratelimitDomain;
        bool notMajorEntity = false; // First snowflake everytime refers to major entityt
                                     // which is part of ratelimit domain.
        for (const std::string& part : splittenPath) {
            ratelimitDomain += "/";
            if ((part.size() >= 17 && part.size() <= 20 && isNumber(part)) ||
                part == "@me") {

                if (notMajorEntity) {
                    ratelimitDomain += "{snowflake}";
                } else {
                    notMajorEntity = true;
                    ratelimitDomain += part;
                }
            } else {
                ratelimitDomain += part;
            }
        }
        return ratelimitDomain;
    }

    RandomSeedGuard::RandomSeedGuard() {
        static bool randomSeeded = false;
        if (!randomSeeded) std::srand(std::time(nullptr));
    }

    std::string randomAsciiString(unsigned length) {
        RandomSeedGuard randGuard;

        constexpr char const asciiCharacters[] = "1234567890abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";

        std::string result;
        result.reserve(length);
        while (result.size() != length) {
            result.push_back(asciiCharacters[std::rand() % sizeof(asciiCharacters)]);
        }
        return result;
    }
}} // namespace Hexicord::Utils
