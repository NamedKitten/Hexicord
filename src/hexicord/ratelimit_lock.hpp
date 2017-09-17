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

#ifndef HEXICORD_RATELIMIT_LOCK_HPP
#define HEXICORD_RATELIMIT_LOCK_HPP

#include <ctime>            // time_t
#include <unordered_map>    // std::unordered_map
#include <string>           // std::string
#include <functional>       // std::function
#include <list>             // std::list

namespace Hexicord
{
    /**
     * Class that implements semaphore-like locking based on Discord's
     * per-route rate limits.
     */
    class RatelimitLock {
    public:
        /**
         * Requests count that can be made using this
         * route until resetTime(route).
         *
         * If not known - returns -1.
         */
        int remaining(const std::string& route);

        /**
         * Latest known maximum count of requests for specified route
         * in current interval. May be outdated due to Discord 
         * __dynamic__ ratelimits.
         *
         * If not known - returns -1.
         */
        int total(const std::string& route);

        /**
         * Unix timestamp when ratelimit will be reset for this route.
         *
         * If not known - returns -1.
         */
        time_t resetTime(const std::string& route);
        
        /**
         * Called before perfoming request, block until reset time if rate limit hit.
         *
         * **Should not be called by user code directly.**
         */
        void down(const std::string& route);

        /**
         * Called after request in order to update information about ratelimits.
         *
         * **Should not be called by user code directly.**
         */
        void refreshInfo(const std::string& route,
                         unsigned remaining,
                         unsigned total,
                         time_t   resetTime);
    private:
        struct RatelimitInfo {
            std::string route;

            unsigned remaining;
            unsigned total;
            time_t resetTime;
        };

        // I need a queue to remove oldest information and 
        // hashmap to provide fast key-indexing.

        std::list<RatelimitInfo> queue;
        std::unordered_map<std::string, decltype(queue)::iterator> ratelimitPointers;
    };
} // namespace Hexicord

#endif // HEXICORD_RATELIMIT_LOCK_HPP
