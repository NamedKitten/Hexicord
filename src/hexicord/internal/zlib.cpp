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

#include "hexicord/internal/zlib.hpp"
#ifdef HEXICORD_ZLIB

#include <cstring> // memcpy, size_t
#include <cassert> // assert
#include <string>  // std::string
#include <zlib.h>  // z_stream inflate inflateEnd

// Closer to trivial message size => better.
constexpr size_t ZlibBufferSize = 16 * 1024;

namespace Hexicord {
namespace Zlib {

    std::vector<uint8_t> decompress(const std::vector<uint8_t>& input) {
        z_stream stream;
        uint8_t in[ZlibBufferSize], out[ZlibBufferSize];
          
        int status;
        stream.zalloc = nullptr;
        stream.zfree = nullptr;
        stream.opaque = nullptr;
        stream.avail_in = 0;
        stream.next_in = nullptr;
        status = inflateInit2(&stream, /* window bits: */ 15);
        assert(status == Z_OK);

        std::vector<uint8_t> result;
        for (uint32_t i = 0; i < input.size(); i += ZlibBufferSize) {
            int left = input.size() - i;
            int wanted = (left > ZlibBufferSize) ? ZlibBufferSize : left;
            std::memcpy(in, input.data()+i, wanted);

            stream.avail_in = wanted;
            stream.next_in = &in[0];
            if (stream.avail_in == 0) break;
            do {
                int have;
                stream.avail_out = ZlibBufferSize;
                stream.next_out = &out[0];

                status = inflate(&stream, Z_NO_FLUSH);
                assert(status != Z_STREAM_ERROR);

                have = ZlibBufferSize - stream.avail_out;
                result.insert(result.end(), out, out + have);
            } while (stream.avail_out == 0);
        }

        inflateEnd(&stream);
        return result;
    }
}
}
#endif
