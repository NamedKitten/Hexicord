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

#include "hexicord/gateway_client.hpp"

#include <cassert>                         // assert
#include <chrono>                          // std::chrono::steady_clock
#include <unordered_map>                   // std::unordered_map
#include <boost/asio/error.hpp>            // boost::asio::error
#include <boost/asio/io_service.hpp>       // boost::asio::io_service
#include <boost/beast/websocket/error.hpp> // boost::beast::websocket::error
#include "hexicord/config.hpp"             // HEXICORD_ZLIB HEXICORD_DEBUG_LOG
#include "hexicord/internal/wss.hpp"       // Hexicord::TLSWebSocket
#include "hexicord/internal/utils.hpp"     // Hexicord::Utils::domainFromUrl

#ifdef HEXICORD_ZLIB
    #include "hexicord/internal/zlib.hpp" // Hexicord::Zlib::decompress
#endif

#ifdef HEXICORD_DEBUG_LOG
    #include <iostream> //
    #define DEBUG_MSG(msg) do { std::cerr <<  "gateway_client.cpp:" << __LINE__ << " " << (msg) << '\n'; } while (false);
#else
    #define DEBUG_MSG(msg)
#endif

// All we need is C++11 compatibile compiler and boost libraries so we can probably run on a lot of platforms.
#if defined(__linux__) || defined(__linux) || defined(linux) || defined(__GNU__)
    #define OS_STR "linux"
#elif defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__) || defined(__DragonFly__)
    #define OS_STR "bsd"
#elif defined(_WIN32) || defined(__WIN32__) || defined(WIN32)
    #define OS_STR "win32"
#elif defined(macintosh) || defined(__APPLE__) || defined(__APPLE_CC__)
    #define OS_STR "macos"
#elif defined(__unix__) || defined (__unix) || defined(_XOPEN_SOURCE) || defined(_POSIX_SOURCE)
    #define OS_STR "unix"
#else
    #define OS_STR "unknown"
#endif

namespace Hexicord {

GatewayClient::GatewayClient(boost::asio::io_service& ioService, const std::string& token)
    : ioService(ioService), token_(token), heartbeatTimer(ioService) {}

GatewayClient::~GatewayClient() {
    if (gatewayConnection && activeSession && gatewayConnection->isSocketOpen()) disconnect(2000);
}

nlohmann::json GatewayClient::parseGatewayMessage(const std::vector<uint8_t>& msg) {
    if (msg.size() == 0) {
        DEBUG_MSG("Got an empty gateway message!");
        return nlohmann::json{};
    }

#ifdef HEXICORD_ZLIB
    if (msg.at(0) == '{') {
        return nlohmann::json::parse(msg);
    }
    return nlohmann::json::parse(Zlib::decompress(msg));
#else
    return nlohmann::json::parse(msg);
#endif
}

void GatewayClient::connect(const std::string& gatewayUrl, int shardId, int shardCount,
                            const nlohmann::json& initialPresence) {

    if (activeSession) disconnect(2000);
    activeSession = false;

    DEBUG_MSG("Connecting...");
    if (!gatewayConnection)                 gatewayConnection.reset(new TLSWebSocket(ioService));
    if (!gatewayConnection->isSocketOpen()) gatewayConnection->handshake(Utils::domainFromUrl(gatewayUrl),
                                                                         gatewayPathSuffix, 443);

    DEBUG_MSG("Reading Hello message...");
    const nlohmann::json gatewayHello = parseGatewayMessage(gatewayConnection->readMessage());

    heartbeatIntervalMs = gatewayHello.at("d").at("heartbeat_interval");
    DEBUG_MSG(std::string("Gateway heartbeat interval: ") + std::to_string(heartbeatIntervalMs) + " ms.");

    heartbeat = true;
    asyncHeartbeat();

    nlohmann::json message = {
        { "token" , token_ },
        { "properties", {
            { "$os", OS_STR },
            { "$browser", "hexicord" },
            { "$device", "hexicord" }
        }},
#ifdef HEXICORD_ZLIB
        { "compress", true },
#else
        { "compress", false },
#endif
        { "large_threshold", 250 }, // should be changeble
        { "presence", initialPresence }
    };

    if (shardId != NoSharding && shardCount != NoSharding) {
        message.push_back({ "shard", { shardId, shardCount }});
    }

    DEBUG_MSG("Sending Identify message...");
    sendMessage(OpCode::Identify, message);

    activeSession = true;

    DEBUG_MSG("Waiting for Ready event...");
    const nlohmann::json readyPayload = waitForEvent(Event::Ready);

    DEBUG_MSG("Got Ready event. Starting heartbeat and polling...");

    // We must dispatch this event too, because it contain
    // information probably useful for users.
    eventDispatcher.dispatchEvent(Event::Ready, readyPayload);

    sessionId_          = readyPayload.at("session_id");
    lastGatewayUrl_     = gatewayUrl;
    shardId_            = shardId;
    shardCount_         = shardCount;
    lastSequenceNumber_ = 0;
    lastPresence        = initialPresence;

    // Already started by waitForEvent.
    // poll = true;
    // asyncPoll();
}

void GatewayClient::resume(const std::string& gatewayUrl,
                           std::string sessionId, int lastSequenceNumber,
                           int shardId, int shardCount) {

    DEBUG_MSG(std::string("Resuming interrupted gateway session. sessionId=") + sessionId +
              " lastSeq=" + std::to_string(lastSequenceNumber));

    if (activeSession) disconnect(2000);
   
    if (!gatewayConnection) gatewayConnection.reset(new TLSWebSocket(ioService));
    if (!gatewayConnection->isSocketOpen()) gatewayConnection->handshake(Utils::domainFromUrl(gatewayUrl), gatewayPathSuffix, 443);

    // FIXME: erroneous double handshake? seems to fail with "unexpected record"
//    DEBUG_MSG("Performing WebSocket handshake...");
//    gatewayConnection->handshake(Utils::domainFromUrl(gatewayUrl), gatewayPathSuffix, 443);

    DEBUG_MSG("Reading Hello message.");
    const nlohmann::json gatewayHello = parseGatewayMessage(gatewayConnection->readMessage());

    heartbeatIntervalMs = gatewayHello.at("d").at("heartbeat_interval");
    DEBUG_MSG(std::string("Gateway heartbeat interval: ") + std::to_string(heartbeatIntervalMs) + " ms.");

    heartbeat = true;
    asyncHeartbeat();

    DEBUG_MSG("Sending Resume message...");
    sendMessage(OpCode::Resume, {
        { "token",      token_             },
        { "session_id", sessionId          },
        { "seq",        lastSequenceNumber }
    });

    DEBUG_MSG("Waiting for Resumed event...");

    activeSession = true;
    // GatewayError can be thrown here, why?
    //  waitForEvent calls processMessage for other messages (including OP Invalid Session),
    //  processMessage throws GatewayError if receives Invalid Session.
    nlohmann::json resumedPayload = waitForEvent(Event::Resumed);
    DEBUG_MSG("Got Resumed event, starting heartbeat and polling...");
    eventDispatcher.dispatchEvent(Event::Resumed, resumedPayload);

    lastGatewayUrl_     = gatewayUrl;
    sessionId_          = sessionId;
    lastSequenceNumber_ = lastSequenceNumber;
}

void GatewayClient::disconnect(int code) noexcept {
    DEBUG_MSG(std::string("Disconnecting from gateway... code=") + std::to_string(code));
    try {
        if (code != NoCloseEvent && !activeSendMessage) sendMessage(OpCode::EventDispatch, nlohmann::json(code), "CLOSE");
    } catch (...) { // whatever happened - we don't care.
    }

    heartbeat = false;
    heartbeatTimer.cancel();

    poll = false;

    gatewayConnection.reset(nullptr);

    activeSession = false;
}

nlohmann::json GatewayClient::waitForEvent(Event type) {
    DEBUG_MSG(std::string("Waiting for event, type=") + std::to_string(unsigned(type)));
    skipMessages = true;

    if (!poll) {
        poll = true;
        asyncPoll();
    }

    while (true) {
        lastMessage = {};

        DEBUG_MSG("Running ASIO event loop iteration...");

        ioService.run_one();

        DEBUG_MSG(lastMessage.dump());

        if (lastMessage.is_null() || lastMessage.empty()) continue;

        if (lastMessage.at("op") == OpCode::EventDispatch &&
            eventEnumFromString(lastMessage.at("t")) == type) {

            break;
        }

        processMessage(lastMessage);
    }

    skipMessages = false;

    return lastMessage.at("d");
}

void GatewayClient::updatePresence(const nlohmann::json& newPresence) {
    sendMessage(OpCode::StatusUpdate, newPresence);
}

void GatewayClient::recoverConnection() {
    DEBUG_MSG("Lost gateway connection, recovering...");
    disconnect(NoCloseEvent);

    try {
        resume(lastGatewayUrl_, sessionId_, lastSequenceNumber_, shardId_, shardCount_);
    } catch (GatewayError& excp) {
        DEBUG_MSG("Resume failed, starting new session...");
        connect(lastGatewayUrl_, shardId_, shardCount_, lastPresence);
    }
}

void GatewayClient::asyncPoll() {
    assert(activeSession);

    DEBUG_MSG("Polling gateway messages...");
    gatewayConnection->asyncReadMessage([this](TLSWebSocket&, const std::vector<uint8_t>& body,
                                               boost::system::error_code ec) {
        if (!poll) return;

        if (ec != boost::system::errc::success) {
            DEBUG_MSG("asyncReadMessage body length: " + std::to_string(body.size()));
            DEBUG_MSG("asyncReadMessage error: " + ec.message());

            // Just reconnect always for now
            // seems like SSL socket can be closed with a short_read error too
            recoverConnection();
/*
            if (ec == boost::asio::error::broken_pipe ||
                ec == boost::asio::error::connection_reset ||
                ec == boost::beast::websocket::error::closed) recoverConnection();
*/
            if (poll) asyncPoll();
            return;
        }

        try {
            const nlohmann::json message = parseGatewayMessage(body);

            lastMessage = message;
            if (!skipMessages
                    && !message.is_null()
                    && !message.empty())
                processMessage(message);
        } catch (nlohmann::json::parse_error& excp) {
            DEBUG_MSG("Corrupted message, assuming connection error, reconnecting...");
            DEBUG_MSG(excp.what());
            DEBUG_MSG(std::string(body.begin(), body.end()));

            // we may fail here because of partially readen message (what
            // means gateway dropped our connection).
            recoverConnection();
        }

        if (poll) asyncPoll();
    });
}

Event GatewayClient::eventEnumFromString(const std::string& str) {
    static const std::unordered_map<std::string, Event> stringToEnum {
        { "READY",                          Event::Ready                },
        { "RESUMED",                        Event::Resumed              },
        { "CHANNEL_CREATE",                 Event::ChannelCreate        },
        { "CHANNEL_UPDATE",                 Event::ChannelUpdate        },
        { "CHANNEL_DELETE",                 Event::ChannelDelete        },
        { "CHANNEL_PINS_CHANGE",            Event::ChannelPinsChange    },
        { "GUILD_CREATE",                   Event::GuildCreate          },
        { "GUILD_UPDATE",                   Event::GuildUpdate          },
        { "GUILD_DELETE",                   Event::GuildDelete          },
        { "GUILD_BAN_ADD",                  Event::GuildBanAdd          },
        { "GUILD_BAN_REMOVE",               Event::GuildBanRemove       },
        { "GUILD_EMOJIS_UPDATE",            Event::GuildEmojisUpdate    },
        { "GUILD_INTEGRATIONS_UPDATE",      Event::GuildIntegrationsUpdate },
        { "GUILD_MEMBER_ADD",               Event::GuildMemberAdd       },
        { "GUILD_MEMBER_REMOVE",            Event::GuildMemberRemove    },
        { "GUILD_MEMBER_UPDATE",            Event::GuildMemberUpdate    },
        { "GUILD_MEMBERS_CHUNK",            Event::GuildMembersChunk    },
        { "GUILD_ROLE_CREATE",              Event::GuildRoleCreate      },
        { "GUILD_ROLE_UPDATE",              Event::GuildRoleUpdate      },
        { "GUILD_ROLE_DELETE",              Event::GuildRoleDelete      },
        { "MESSAGE_CREATE",                 Event::MessageCreate        },
        { "MESSAGE_UPDATE",                 Event::MessageUpdate        },
        { "MESSAGE_DELETE",                 Event::MessageDelete        },
        { "MESSAGE_DELETE_BULK",            Event::MessageDeleteBulk    },
        { "MESSAGE_REACTION_ADD",           Event::MessageReactionAdd   },
        { "MESSAGE_REACTION_REMOVE_ALL",    Event::MessageReactionRemoveAll },
        { "PRESENCE_UPDATE",                Event::PresenceUpdate       },
        { "TYPING_START",                   Event::TypingStart          },
        { "USER_UPDATE",                    Event::UserUpdate           },
        { "VOICE_STATE_UPDATE",             Event::VoiceStateUpdate     },
        { "VOICE_SERVER_UPDATE",            Event::VoiceServerUpdate    },
        { "WEBHOOKS_UPDATE",                Event::WebhooksUpdate       }
    };

    auto it = stringToEnum.find(str);
    if (it == stringToEnum.end())
        return Event::Unknown;

    return it->second;
}

void GatewayClient::processMessage(const nlohmann::json& message) {
    switch (message.at("op").get<int>()) {
    case OpCode::EventDispatch:
    {
        const auto &t = message.at("t");
        const auto &s = message.at("s");
        const auto &d = message.at("d");
        DEBUG_MSG(std::string("Gateway Event: t=") + t.get<std::string>() +
                  " s=" + std::to_string(s.get<int>()));
        lastSequenceNumber_ = s.get<int>();
        eventDispatcher.dispatchEvent(eventEnumFromString(t), d);
        break;
    }
    case OpCode::HeartbeatAck:
        assert(activeSession);
        DEBUG_MSG("Gateway heartbeat answered.");
        --unansweredHeartbeats;
        break;
    case OpCode::Heartbeat:
        assert(activeSession);
        DEBUG_MSG("Received heartbeat request.");
        sendMessage(OpCode::Heartbeat, nlohmann::json(lastSequenceNumber_));
        ++unansweredHeartbeats;
        break;
    case OpCode::Reconnect:
        assert(activeSession);
        DEBUG_MSG("Gateway asked us to reconnect...");
        // It's not recoverConnection duplicate, here Invalid Session error during
        // resume is real error, not just 'start new session instead'.
        disconnect();
        resume(lastGatewayUrl_, sessionId_, lastSequenceNumber_);
        break;
    case OpCode::InvalidSession:
        DEBUG_MSG("Invalid session error.");
        throw GatewayError("Invalid session.");
        break;
    default:
        DEBUG_MSG("Unexpected gateway message.");
        DEBUG_MSG(message);
    }
}

void GatewayClient::sendMessage(GatewayClient::OpCode opCode, const nlohmann::json& payload, const std::string& t) {
    nlohmann::json message = {
        { "op", opCode  },
        { "d",  payload },
    };

    if (!t.empty()) {
        message["t"] = t;
    }

    std::string messageString = message.dump();

    activeSendMessage = true;
    gatewayConnection->sendMessage(std::vector<uint8_t>(messageString.begin(), messageString.end()));
    activeSendMessage = false;
}

void GatewayClient::asyncHeartbeat() {
    heartbeatTimer.cancel();
    heartbeatTimer.expires_from_now(std::chrono::milliseconds(heartbeatIntervalMs));
    heartbeatTimer.async_wait([this](const boost::system::error_code& ec){
        if (ec == boost::asio::error::operation_aborted) return;
        if (!heartbeat) return;

        sendHeartbeat();

        asyncHeartbeat();
    });
}

void GatewayClient::sendHeartbeat() {
    assert(activeSession);
    if (unansweredHeartbeats >= 2) {
        DEBUG_MSG("Missing gateway heartbeat answer. Reconnecting...");
        recoverConnection();
        return;
    }

    DEBUG_MSG("Gateway heartbeat sent.");
    sendMessage(OpCode::Heartbeat, nlohmann::json(lastSequenceNumber_));
    ++unansweredHeartbeats;
}

} // namespace Hexicord
