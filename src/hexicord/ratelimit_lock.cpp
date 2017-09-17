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

#include "hexicord/ratelimit_lock.hpp"

#include <chrono>               // std::chrono::seconds
#include <thread>               // std::this_thread::sleep_for
#include "hexicord/config.hpp"  // HEXICORD_DEBUG_LOG

#ifdef HEXICORD_DEBUG_LOG
    #include <iostream>
    #define DEBUG_MSG(msg) do { std::cerr << "ratelimit_lock.cpp:" << __LINE__ << "\t" << (msg) << '\n'; } while (false)
#else
    #define DEBUG_MSG(msg)
#endif

int Hexicord::RatelimitLock::remaining(const std::string& route) {
    auto it = ratelimitPointers.find(route);
    return it != ratelimitPointers.end() ? it->second->remaining : -1;
}

int Hexicord::RatelimitLock::total(const std::string& route) {
    auto it = ratelimitPointers.find(route);
    return it != ratelimitPointers.end() ? it->second->total : -1;
}

time_t Hexicord::RatelimitLock::resetTime(const std::string& route) {
    auto it = ratelimitPointers.find(route);
    return it != ratelimitPointers.end() ? it->second->resetTime : -1;
}

void Hexicord::RatelimitLock::down(const std::string& route) {
    auto it = ratelimitPointers.find(route);

    // We can't predict limit hit in this case, so assume we don't hit it.
    if (it == ratelimitPointers.end()) {
        DEBUG_MSG(std::string("Can't predict hit for route (no information) ") + route);
        return;
    }

    RatelimitInfo& routeInfo = *it->second;

    --routeInfo.remaining;

    DEBUG_MSG(std::string("Ratelimit semaphore acquire for route ") + route +
              " total=" + std::to_string(routeInfo.total) + 
              ", remaining=" + std::to_string(routeInfo.remaining));

    if (routeInfo.remaining == 0) {
        if (routeInfo.resetTime <= std::time(nullptr)) {
            DEBUG_MSG(std::string("Ratelimit information for route ") + route + " is outdated, can't predict hit!");
            queue.erase(it->second);
            ratelimitPointers.erase(it);
            return;
        }

        DEBUG_MSG(std::string("Ratelimit hit for route ") + route + ", blocking until " +
                  std::to_string(routeInfo.resetTime));

        std::this_thread::sleep_for(std::chrono::seconds(routeInfo.resetTime - std::time(nullptr)));

        // we also erase information after, so it can't be outdated.
        queue.erase(it->second);
        ratelimitPointers.erase(it);
    }
}

void Hexicord::RatelimitLock::refreshInfo(const std::string& route,
                                          unsigned remaining,
                                          unsigned total,
                                          time_t   resetTime) {

    DEBUG_MSG(std::string("Route ") + route +
              ": remaining=" + std::to_string(remaining) +
              ", total=" + std::to_string(total) +
              ", resetTime=" + std::to_string(resetTime));

    auto ratelimitItIt = ratelimitPointers.find(route);
    if (ratelimitItIt != ratelimitPointers.end()) {
        queue.push_back({ route, remaining, total, resetTime });
    } else {
        if (queue.size() == HEXICORD_RATELIMIT_CACHE_SIZE) {
            ratelimitPointers.erase(ratelimitPointers.find(queue.front().route));
            
            DEBUG_MSG("Ratelimit cache hit HEXICORD_RATELIMIT_CACHE_SIZE, earsing information about oldest route...");
            DEBUG_MSG(std::string("Route removed: ") + queue.front().route);

            queue.pop_front();
        }

        queue.push_back({ route, remaining, total, resetTime });
        ratelimitPointers.insert({route, --queue.end() });
    }
}
