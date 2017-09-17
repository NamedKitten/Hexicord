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

#ifndef HEXICORD_EVENTDISPATCHER_HPP
#define HEXICORD_EVENTDISPATCHER_HPP 

#include <unordered_map>        // std::unordered_map
#include <functional>           // std::function
#include <cstddef>              // size_t
#include <string>               // std::string
#include <vector>               // std::vector
#include "hexicord/json.hpp"    // nlohmann::json

namespace Hexicord {
    enum class Event {
        Ready,
        Resumed,
        ChannelCreate,
        ChannelUpdate,
        ChannelDelete,
        ChannelPinsChange,
        GuildCreate,
        GuildUpdate,
        GuildDelete,
        GuildBanAdd,
        GuildBanRemove,
        GuildEmojisUpdate,
        GuildIntegrationsUpdate,
        GuildMemberAdd,
        GuildMemberRemove,
        GuildMemberUpdate,
        GuildMembersChunk,
        GuildRoleCreate,
        GuildRoleUpdate,
        GuildRoleDelete,
        MessageCreate,
        MessageUpdate,
        MessageDelete,
        MessageDeleteBulk,
        MessageReactionAdd,
        MessageReactionRemoveAll,
        PresenceUpdate,
        TypingStart,
        UserUpdate,
        VoiceStateUpdate,
        VoiceServerUpdate,
        WebhooksUpdate,
    };

    struct EventHash {
        inline std::size_t operator()(Event e) const noexcept {
            return static_cast<std::size_t>(e);
        }
    };

    class EventDispatcher {
    public:
        using EventHandler        = std::function<void(const nlohmann::json&)>;
        using UnknownEventHandler = std::function<void(const std::string&, const nlohmann::json&)>;

        void addHandler(Event eventType, const EventHandler& handler);

        void dispatchEvent(Event type, const nlohmann::json& payload) const;
    private:
        static const std::unordered_map<std::string, Event> stringToEnum;

        std::unordered_map<Event, std::vector<EventHandler>, EventHash> handlers {
            { Event::Ready, {} },
            { Event::Resumed, {} },
            { Event::ChannelCreate, {} },
            { Event::ChannelUpdate, {} },
            { Event::ChannelDelete, {} },
            { Event::ChannelPinsChange, {} },
            { Event::GuildCreate, {} },
            { Event::GuildUpdate, {} },
            { Event::GuildDelete, {} },
            { Event::GuildBanAdd, {} },
            { Event::GuildBanRemove, {} },
            { Event::GuildEmojisUpdate, {} },
            { Event::GuildIntegrationsUpdate, {} },
            { Event::GuildMemberAdd, {} },
            { Event::GuildMemberRemove, {} },
            { Event::GuildMemberUpdate, {} },
            { Event::GuildMembersChunk, {} },
            { Event::GuildRoleCreate, {} },
            { Event::GuildRoleUpdate, {} },
            { Event::GuildRoleDelete, {} },
            { Event::MessageCreate, {} },
            { Event::MessageUpdate, {} },
            { Event::MessageDelete, {} },
            { Event::MessageDeleteBulk, {} },
            { Event::MessageReactionAdd, {} },
            { Event::MessageReactionRemoveAll, {} },
            { Event::PresenceUpdate, {} },
            { Event::TypingStart, {} },
            { Event::UserUpdate, {} },
            { Event::VoiceStateUpdate, {} },
            { Event::VoiceServerUpdate, {} },
            { Event::WebhooksUpdate, {} }
        };

        std::vector<UnknownEventHandler> unknownEventHandlers;

    };
} // namespace Hexicord

#endif // HEXICORD_EVENTDISPATCHER_HPP
