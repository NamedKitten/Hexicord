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

#ifndef HEXICORD_REST_CLIENT_HPP
#define HEXICORD_REST_CLIENT_HPP

#include <cstdint>                      // uint8_t
#include <utility>                      // std::pair
#include <string>                       // std::string
#include <vector>                       // std::vector
#include <unordered_map>                // std::unordered_map
#include <memory>                       // std::shared_ptr
#include <boost/optional.hpp>           // boost::optional
#include "hexicord/json.hpp"            // nlohamnn::json
#include "hexicord/permission.hpp"      // Hexicord::Permissions
#include "hexicord/config.hpp"          // HEXICORD_RATELIMIT_PREDICTION
#include "hexicord/types.hpp"           // Hexicord::Snowflake, Hexicord::File, Hexicord::Image
namespace boost { namespace asio { class io_service; }}
namespace Hexicord { namespace REST { class HTTPSConnection; class MultipartEntity; class HTTPRequest; class HTTPResponse; }}
#ifdef HEXICORD_RATELIMIT_PREDICTION
    #include "hexicord/ratelimit_lock.hpp"
#endif

namespace Hexicord {

    class RestClient {
    public:
        /**
         * Construct RestClient, does nothing network-related to make RestClient's cheap
         * to construct.
         *
         * \param ioService ASIO I/O service. Should not be destroyed while
         *                  RestClient exists.
         * \param token     token string, will be interpreted as OAuth or Bot token
         *                  depending on future calls, don't add "Bearer " or
         *                  "Bot " prefix.
         */
        RestClient(boost::asio::io_service& ioService, const std::string& token);

        RestClient(const RestClient&) = delete;
        RestClient(RestClient&&) = default;

        RestClient& operator=(const RestClient&) = delete;
        RestClient& operator=(RestClient&&) = default;

        /**
         * Returns gateway URL to be used with \ref GatewayClient::connect.
         *
         * Client expected to cache this URL and request new only
         * if fails to use old one.
         *
         * \sa \ref getGatewayUrlBot
         */
        std::string getGatewayUrl();

        /**
         * Return gateway URL to be used with \ref GatewayClient::connect.
         * if client is a bot. Also returns recommended shards count.
         *
         * \returns std::pair with gateway URL (first) and recommended
         *          shards count (second).
         *
         * \sa \ref getGatewayUrl
         */
        std::pair<std::string, int> getGatewayUrlBot();

        /**
         * Send raw REST-request and return result json.
         *
         * \note Newly constructed RestClient object don't have open REST
         *       connection. It will be openned when restRequest called
         *       first time.
         *
         * \param method    used HTTP method, can be any string without spaces
         *                  but following used by Discord API: "POST", "GET",
         *                  "PATCH", "PUT", "DELETE".
         * \param endpoint  endpoint URL relative to base URL (including leading slash).
         * \param payload   JSON payload, pass empty object (default) if none.
         * \param query     GET request query.
         * \param multipart Pass one or more MultipartEntity to perform multipart request.
         *                  If payload is also present, it will be first entity.
         *
         * \ingroup REST
         */
        nlohmann::json sendRestRequest(const std::string& method, const std::string& endpoint,
                                       const nlohmann::json& payload = {},
                                       const std::unordered_map<std::string, std::string>& query = {},
                                       const std::vector<REST::MultipartEntity>& multipart = {});

        /** \defgroup REST REST methods
         *
         * Functions for performing requests to REST endpoints.
         *
         * All methods in this group is thread-safe if not stated otherwise.
         * All methods in this group may throw RESTError (or any subclass) and boost::system::system_error.
         *
         * @{
         */

        /**
         * \defgroup REST_channels Channel operations
         *
         * Methods related to channels.
         *
         * Most require MANAGE_CHANNELS permission if operating on guild channel.
         *
         * @{
         */

        /**
         * Get a channel by ID. Returns a guild channel or dm channel object.
         */
        nlohmann::json getChannel(Snowflake channelId);

        /**
         * Update a channels settings.
         *
         * Requires the MANAGE_CHANNELS permission for the guild.
         *
         * \param channelId  Snowflake ID of target channel.
         * \param name       2-100 character channel name.
         * \param position   The position of the channel in the left-hand listing.
         * \param topic      0-1024 character channel topic (Text channel only).
         * \param bitrate    The bitrate (in bits) for voice channel; 8000 to 96000
         *                   (to 128000 for VIP servers).
         * \param usersLimit The user limit for voice channels; 0-99, 0 means no limit.
         *
         * \returns Guild channel object after modification.
         */
        nlohmann::json modifyChannel(Snowflake channelId,
                                     boost::optional<std::string> name = boost::none,
                                     boost::optional<int> position = boost::none,
                                     boost::optional<std::string> topic = boost::none,
                                     boost::optional<unsigned> bitrate = boost::none,
                                     boost::optional<uint16_t> usersLimit = boost::none);

        /// Same as \def modifyChannel, but changes only name.
        inline nlohmann::json setChannelName(Snowflake channelId, const std::string& newName) {
            return modifyChannel(channelId, newName);
        }
        /// Same as \def modifyChannel, but changes only position.
        inline nlohmann::json setChannelPosition(Snowflake channelId, int newPosition) {
            return modifyChannel(channelId, boost::none, newPosition);
        }
        /// Same as \def modifyChannel, but changes only topic.
        inline nlohmann::json setChannelTopic(Snowflake textChannelId, const std::string& newTopic) {
            return modifyChannel(textChannelId, boost::none, boost::none, newTopic);
        }
        /// Same as \def modifyChannel, but changes only bitrate.
        inline nlohmann::json setChannelBitrate(Snowflake voiceChannelId, unsigned newBitrate) {
            return modifyChannel(voiceChannelId, boost::none, boost::none, boost::none, newBitrate);
        }
        /// Same as \def modifyChannel, but changes only users limit.
        inline nlohmann::json setChannelUsersLimit(Snowflake voiceChannelId, uint16_t newLimit) {
            return modifyChannel(voiceChannelId, boost::none, boost::none, boost::none, boost::none, newLimit);
        }

        /**
         * Delete a guild channel or close DM.
         *
         * Requires the MANAGE_CHANNELS permission for guild.
         *
         * \returns Channel object.
         */
        nlohmann::json deleteChannel(Snowflake channelId);

        /**
         * \class After
         * Tag types for \ref getMessages.
         *
         * \class Before
         * Tag types for \ref getMessages.
         *
         * \class Around
         * Tag types for \ref getMessages.
         */

        struct After { Snowflake id; };
        struct Before { Snowflake id; };
        struct Around { Snowflake id; };

        /**
         * Get messages after specified id.
         *
         * Limit can't be larger than 100, default is 50.
         */
        nlohmann::json getMessages(Snowflake channelId, After afterId, unsigned limit = 50);

        /**
         * Get messages before specified id.
         *
         * Limit can't be larger than 100, default is 50.
         */
        nlohmann::json getMessages(Snowflake channelId, Before beforeId, unsigned limit = 50);

        /**
         * Get messages around specified id.
         *
         * Limit can't be larger than 100 or smaller than 2, default is 50.
         */
        nlohmann::json getMessages(Snowflake channelId, Around aroundId, unsigned limit = 50);

        /// Same as \ref getMessage.
        inline nlohmann::json getMessages(Snowflake channelId, Snowflake messageId) {
            return getMessage(channelId, messageId);
        }

        /**
         * Get single message.
         */
        nlohmann::json getMessage(Snowflake channelId, Snowflake messageId);

        /// \defgroup REST_pins Pinned messages operations
        /// Methods related to pinned messages.
        /// @{

        /**
         * Returns JSON array of all pinned messages in channel
         * specified by channelId.
         */
        nlohmann::json getPinnedMessages(Snowflake channelId);

        /**
         * Pin message messageId in channel channelId.
         *
         * Requires MANAGE_MESSAGES permission.
         */
        void pinMessage(Snowflake channelId, Snowflake messageId);

        /**
         * Unpin message messageId in channel channelId.
         *
         * Requires MANAGE_MESSAGES permission.
         */
        void unpinMessage(Snowflake channelId, Snowflake messageId);

        /// @} REST_pins

        /**
         * Change existing or add new channel permission override for a role.
         *
         * Only usable for guild channels. Requires the 'MANAGE_ROLES' permission.
         *
         * If you don't specify permission in either allow or deny parmeter it will
         * inherit global value (no override). Adding permission to both allow and
         * deny is not allowed and will throw RESTError.
         */
        void editChannelRolePermissions(Snowflake channelId, Snowflake roleId,
                                        Permissions allow, Permissions deny);

        /**
         * Change existing or add new channel permission override for a user.
         *
         * Only usable for guild channels. Requires the 'MANAGE_ROLES' permission.
         *
         * If you don't specify permission in either allow or deny parmeter it will
         * inherit global value (no override). Adding permission to both allow and
         * deny is not allowed and will throw RESTError.
         */
        void editChannelUserPermissions(Snowflake channelId, Snowflake userId,
                                        Permissions allow, Permissions deny);

        /**
         * Delete a channel permission overwrite for a user or role in a channel.
         * overrideId may be role ID or user ID, depending on override type.
         *
         * Only usable for guild channels. Requires the 'MANAGE_ROLES' permission.
         */
        void deleteChannelPermissions(Snowflake channelId, Snowflake overrideId);

        /**
         * Remove (kick) member from group DM.
         *
         * \sa \ref addToGroupDm
         */
        void kickFromGroupDm(Snowflake groupDmId, Snowflake userId);

        /**
         * Add member to group DM.
         *
         * You should pass access token of a user that has granted
         * your app the gdm.join scope and nickname in order
         * to add him.
         *
         * \sa \ref kickFromGroupDm
         */
        void addToGroupDm(Snowflake groupDmId, Snowflake userId,
                          const std::string& accessToken, const std::string& nick);

        void triggerTypingIndicator(Snowflake channelId);

        /// @} Channel methods

        /**
         * \defgroup REST_messages Messages operations
         *
         * Methods related to messages.
         *
         * @{
         */

        /**
         * Send a text message to a text channel (or DM).
         *
         * Requires SEND_MESSAGE permission if operating on guild channel.
         *
         * You can set tts to true, if you want to send
         * Text-To-Speech messsage.
         *
         * text can't be bigger than 200 characters.
         *
         * \returns Message object that represents sent message.
         *
         * \sa \ref sendFile
         */
        nlohmann::json sendTextMessage(Snowflake channelId, const std::string& text,
                                       const nlohmann::json& embed = nullptr, bool tts = false);

        /**
         * Send a message with a file to a text channel (or DM).
         *
         * \warning Regular accounts and bots have a limit of file size (8 MB),
         *         user accounts with Discord Nitro have 50 MB limit. If you send
         *         a file bigger than allowed, you will get RESTError with 40005 code.
         *
         * \returns Message object that represents sent message.
         *
         * \sa \ref sendTextMessage
         */
        nlohmann::json sendFile(Snowflake channelId, const File& file);

        /**
         * Same as \ref sendFile.
         */
        inline nlohmann::json sendImage(Snowflake channelId, const Image& image) {
            return sendFile(channelId, image.file);
        }

        /**
         * Edit a previously sent message. You can only edit messages that have
         * been sent by the current user.
         *
         * \returns Message object after change.
         */
        nlohmann::json editMessage(Snowflake channelId, Snowflake messageId,
                                   const std::string& text, const nlohmann::json& embed = nullptr);

        /**
         * Delete a message. If operating on a guild channel and trying to
         * delete a message that was not sent by the current user,
         * requires the 'MANAGE_MESSAGES' permission.
         *
         * \sa \ref deleteMessages
         */
        void deleteMessage(Snowflake channelId, Snowflake messageId);

        /**
         * Delete multiple messages in a single request. This endpoint can
         * only be used on guild channels and requires
         * the 'MANAGE_MESSAGES' permission.
         *
         * \warning This method will not delete messages older than 2 weeks,
         * and will fail if any message provided is older than that.
         *
         * \sa \ref deleteMessage
         */
        void deleteMessages(Snowflake channelId, const std::vector<Snowflake>& messageIds);

        /// \defgroup REST_reactions Reactions
        /// Methods related to message reactions.
        /// @{

        /**
         * Add reaction to message.
         *
         * This method requires READ_MESSAGE_HISTORY permission and,
         * additionally, ADD_REACTIONS permissions if nobody else reacted
         * using this emoji.
         *
         * \sa \ref removeReaction \ref getReactions
         */
        void addReaction(Snowflake channelId, Snowflake messageId, Snowflake emojiId);

        /**
         * Remove reaction from message.
         *
         * By default this method removes current user's reaction, pass userId
         * to remove other user's reactions, however this requires
         * MANAGE_MESSAGES permission.
         *
         * \sa \ref addReaction \ref getReactions
         */
        void removeReaction(Snowflake channelId, Snowflake messageId, Snowflake emojiId, Snowflake userId = 0);

        /**
         * Get a list of users that reacted with this emoji.
         *
         * \sa \ref addReaction \ref removeReaction
         */
        nlohmann::json getReactions(Snowflake channelId, Snowflake messageId, Snowflake emojiId);

        /**
         * Remove all reactions from message. Requires MANAGE_MESSAGES
         * permission.
         */
        void resetReactions(Snowflake channelId, Snowflake messageId);

        /// @} REST_reactions

        /// @} REST_messages

        /// \defgroup REST_guilds Guild methods
        /// Methods related to guilds.
        /// @{

        /**
         * Get guild object by id.
         */
        nlohmann::json getGuild(Snowflake id);

        /**
         * Create a new guild. Returns a guild object.
         *
         * See https://discordapp.com/developers/docs/resources/guild#create-guild-json-params
         * for list of fields to pass.
         *
         * \warning By default this endpoint is limited to 10 active guilds.
         * These limits are raised for whitelisted GameBridge applications.
         *
         * \note If roles are specified, the required id field within each
         * role object is an integer placeholder, and will be replaced by
         * the API upon consumption. Its purpose is to allow you to overwrite
         * a role's permissions in a channel when also passing in channels
         * with the channels array.
         */
        nlohmann::json createGuild(const nlohmann::json& newGuildObject);

        /**
         * Modify a guild's settings. Returns the updated guild object.
         *
         * Pass only changed settings, no need to pass everything.
         */
        nlohmann::json modifyGuild(Snowflake id, const nlohmann::json& changedFields);

        /**
         * Get guild bans. Returns array of ban objects.
         *
         * Requires BAN_MEMBERS permission.
         */
        nlohmann::json getBans(Snowflake guildId);

        /**
         * Ban member on guild.
         *
         * Requires BAN_MEMBERS permissions.
         * You can additionally specify deleteMessageDays to remove user
         * messages for last (0-7) days.
         */
        void banMember(Snowflake guildId, Snowflake userId, unsigned deleteMessagesDays = 0);

        /**
         * Unban member on guild.
         *
         * Requires BAN_MEMBERS permissions.
         */
        void unbanMember(Snowflake guildId, Snowflake userId);

        /**
         * Remove a member from a guild.
         *
         * Requires 'KICK_MEMBERS' permission.
         */
        void kickMember(Snowflake guildId, Snowflake userId);

        /**
         * Get channels for specified guild.
         */
        nlohmann::json getChannels(Snowflake guildId);

        /**
         * Create new channel.
         *
         * Requires MANAGE_CHANNELS permission.
         *
         * channelField needs to be filled according to https://discordapp.com/developers/docs/resources/guild#create-guild-channel-json-params
         */
        nlohmann::json createChannel(Snowflake guildId, const nlohmann::json& channelFields);

        /**
         * Change order of channels.
         *
         * Requires MANAGE_CHANNELS permission.
         *
         * \note Only channels to be modified are required, with the minimum
         * being a swap between at least two channels.
         */
        void reorderChannels(Snowflake guildId, const std::vector<std::pair<Snowflake, unsigned>>& newPositions);

        /**
         * Change order of roles.
         *
         * Requires MANAGE_ROLES permission.
         */
        void reorderRoles(Snowflake guildId, const std::vector<std::pair<Snowflake, unsigned>>& newPositions);

        /// Same as reorderChannels with one pair passed.
        inline void moveChannel(Snowflake guildId, Snowflake channelId, unsigned newPosition) {
            reorderChannels(guildId, {{ channelId, newPosition }});
        }

        /// Same as reorderRoles with one pair passed.
        inline void moveRole(Snowflake guildId, Snowflake roleId, unsigned newPosition) {
            reorderRoles(guildId, {{ roleId, newPosition }});
        }

        /**
         * Get roles for a guild.
         */
        nlohmann::json getRoles(Snowflake guildId);

        /**
         * Create new role for a guild.
         *
         * Requires MANAGE_ROLES permission.
         *
         * roleObject needs to be filled according to https://discordapp.com/developers/docs/resources/guild#create-guild-role-json-params
         */
        nlohmann::json createRole(Snowflake guildId, const nlohmann::json& roleObject);

        /**
         * Modify a role.
         *
         * Requires MANAGE_ROLES permission.
         *
         * updatedFields needs to be filled according to https://discordapp.com/developers/docs/resources/guild#modify-guild-role-json-params
         */
        nlohmann::json modifyRole(Snowflake guildId, Snowflake roleId, const nlohmann::json& updatedFields);

        /**
         * Delete a role.
         *
         * Requires MANAGE_ROLES permissions.
         */
        void deleteRole(Snowflake guildId, Snowflake roleId);

        /**
         * Get JSON array of guild members.
         * Returns guild member objects as described at
         * https://discordapp.com/developers/docs/resources/guild#GUILD/guild-member-object
         *
         * limit is maximum amount of members to return (1-1000).
         *
         * after is exactly what you may think: "return members after this id". Discord
         * docs say "the highest user id in the previous page", but we know that returned
         * array is sorted by ids, so basicly it's same as "last user id in the
         * previous page".
         */
        nlohmann::json getMembers(Snowflake guildId, unsigned limit = 100, Snowflake after = 0);

        /**
         * Return guild member object as described at
         * https://discordapp.com/developers/docs/resources/guild#GUILD/guild-member-object
         */
        nlohmann::json getMember(Snowflake guildId, Snowflake userId);

        /**
         * Change user nickname for a guild.
         *
         * Requires MANAGE_NICKNAMES permission.
         */
        void setMemberNickname(Snowflake guildId, Snowflake userId, const std::string& newNick);

        /**
         * Set guild member roles.
         *
         * Requires MANAGE_ROLES permission.
         */
        void setMemberRoles(Snowflake guildId, Snowflake userId, const std::vector<Snowflake>& newRoles);

        /**
         * Mute/unmute member.
         *
         * Requires MUTE_MEMBERS permission.
         */
        void setMemberMute(Snowflake guildId, Snowflake userId, bool muted = true);

        /// Same as setMemberMute(guildId, userId, true);
        inline void muteMember(Snowflake guildId, Snowflake userId) {
            setMemberMute(guildId, userId, true);
        }

        /// Same as setMemberMute(guildId, userId, true);
        inline void unmuteMember(Snowflake guildId, Snowflake userId) {
            setMemberMute(guildId, userId, false);
        }

        /**
         * Deaf/undeaf member.
         *
         * Requires DEAFEN_MEMBERS permission.
         */
        void setMemberDeaf(Snowflake guildId, Snowflake userId, bool deafen = true);

        // Same as setMemberDeaf(guildId, userId, true);
        inline void deafMember(Snowflake guildId, Snowflake userId) {
            setMemberDeaf(guildId, userId, true);
        }

        // Same as setMemberDeaf(guildId, userId, true);
        inline void undeafMember(Snowflake guildId, Snowflake userId) {
            setMemberDeaf(guildId, userId, false);
        }

        /**
         * Move member to voice channel.
         *
         * Can only be used if member already connected to voice. API must have
         * permission to connect to target channel and MOVE_MEMBERS permission.
         */
        void moveMember(Snowflake guildId, Snowflake userId, Snowflake targetChannel);

        /**
         * Returns a list of integration objects for the guild.
         *
         * Requires the 'MANAGE_GUILD' permission.
         */
        nlohmann::json getGuildIntegrations(Snowflake guildId);

        /**
         * Attach an integration object from the current user to the guild.
         *
         * Requires the 'MANAGE_GUILD' permission.
         */
        void attachIntegration(Snowflake guildId, const std::string& type, Snowflake integrationId);

        /**
         * See https://discordapp.com/developers/docs/resources/guild#modify-guild-integration
         * There would be better documentation, but we don't know what these fields mean.
         */
        void modifyAttachedIntegration(Snowflake guildId, Snowflake integrationId,
                                       int expireBehavior, int expireGracePerior, bool enableEmoticons);

        /**
         * Delete the attached integration object for the guild.
         *
         * Requires the 'MANAGE_GUILD' permission.
         */
        void detachIntegration(Snowflake guildId, Snowflake integrationId);

        /**
         * Sync an integration.
         *
         * Requires the 'MANAGE_GUILD' permission.
         */
        void syncIntegration(Snowflake guildId, Snowflake integrationId);

        /**
         * Get guild embed object.
         *
         * Requires the 'MANAGE_GUILD' permission.
         */
        nlohmann::json getGuildEmbed(Snowflake guildId);

        /**
         * Update embed object for the guild.
         *
         * Requires the 'MANAGE_GUILD' permission.
         */
        void modifyGuildEmbed(Snowflake guildId, bool enabled, Snowflake channelId);

        /**
         * Adds a role to a guild member.
         *
         * Requires the 'MANAGE_ROLES' permission.
         */
        void giveRole(Snowflake guildId, Snowflake userId, Snowflake roleId);

        /**
         * Removes a role from a guild member.
         *
         * Requires the 'MANAGE_ROLES' permission.
         */
        void takeRole(Snowflake guildId, Snowflake userId, Snowflake roleId);

        /**
         * Get number of members that would be removed in a prune operation.
         *
         * Requires 'KICK_MEMBERS' permission.
         */
        unsigned getGuildPruneCount(Snowflake guildId, unsigned days);

        /**
         * Begin a prune operation.
         *
         * Requires the 'KICK_MEMBERS' permission.
         *
         * Returns value indicating the number of members that
         * were removed in the prune operation.
         */
        unsigned beginGuildPrune(Snowflake guildId, unsigned days);

        /// @} REST_guilds

        /**
         * \defgroup REST_users Users methods
         *
         * Methods related to users.
         *
         * @{
         */

        /**
         * Returns the user object of the requester's account.
         *
         * For OAuth2, this requires the identify scope, which will return
         * the object without an email, and optionally the email scope,
         * which returns the object with an email.
         */
        nlohmann::json getMe();

        /**
         * Return a user object for gived user ID.
         */
        nlohmann::json getUser(Snowflake id);

        /**
         * Change username.
         *
         * May cause discriminator to be randomized.
         *
         * \param newUsername  New username.
         * Discord enforces the following restrictions for usernames and nicknames:
         * * Names can contain most valid unicode characters. We limit some
         *   zero-width and non-rendering characters.
         * * Names must be between 2 and 32 characters long.
         * * Names cannot contain the following substrings: '@', '#',
         *   ':', '```'.
         * * Names cannot be: 'discordtag', 'everyone', 'here'.
         * * Names are sanitized and trimmed of leading, trailing, and
         *   exessive internal whitespace.
         * If first or third restriction violated, InvalidParameter will be thrown.
         *
         * There are other rules and restrictions not shared here for the sake
         * of spam and abuse mitigation, but the majority of users won't
         * encounter them. It's important to properly handle all error
         * messages returned by Discord when editing or updating names.
         *
         * \returns User object after change.
         */
        nlohmann::json setUsername(const std::string& newUsername);

        /**
         * Change avatar.
         *
         * \returns User object after change.
         */
        nlohmann::json setAvatar(const Image& avatar);

        /**
         * Returns list of partial guild object the current user is member of.
         *
         * Requires guilds OAuth2 scope.
         *
         * Example of partial guild object:
         * ```json
         * {
         *     "id": "80351110224678912",
         *     "name": "1337 Krew",
         *     "icon": "8342729096ea3675442027381ff50dfe",
         *     "owner": true,
         *     "permissions": 36953089
         * }
         * ```
         * Fields have same meaning as regular in full guild object.
         *
         * \note This methods returns 100 guilds by default, which is the
         * maximum number of guilds a non-bot user can join. Therefore,
         * pagination is not needed for integrations that need to get a
         * list of users' guilds.
         *
         * \param limit    Max number of guilds to return (1-100).
         * \param startId  Get guilds **after** this guild ID.
         * \param before   Get guilds **before** startId, not after.
         */
        nlohmann::json getUserGuilds(uint16_t limit = 100, Snowflake startId = 0, bool before = false);

        /**
         * Leave a guild.
         */
        void leaveGuild(Snowflake guildId);

        /**
         * Returns a list of DM channel objects.
         */
        nlohmann::json getUserDms();

        /**
         * Create a new DM channel with a user.
         *
         * \returns Created DM channel object.
         */
        nlohmann::json createDm(Snowflake recipientId);

        /**
         * Create a new group DM channel with multiple users. Returns
         * a DM channel object.
         *
         * \warning By default this method is limited to 10 active group DMs.
         * These limits are raised for whitelisted GameBridge applications.
         *
         * \param accessTokens Access tokens of users that have granted your
         *                     app the gdm.join scope.
         * \param nicks        A dictionary of user ids to their respective nicknames.
         */
        nlohmann::json createGroupDm(const std::vector<Snowflake>& accessTokens,
                                     const std::unordered_map<Snowflake, std::string>& nicks);

        /**
         * Returns a list of connection objects.
         *
         * Requires the connections OAuth2 scope.
         */
        nlohmann::json getConnections();

        /// @} REST_users

        /**
         * \defgroup REST_invites Invites operations
         *
         * Methods related to invites.
         *
         * @{
         */

        /**
         * Get array of invite objects (with invite metadata) for a specified guild.
         *
         * Requires MANAGE_GUILD permission.
         */
        nlohmann::json getInvites(Snowflake guildId);

        /**
         * Returns an invite object for the given code.
         */
        nlohmann::json getInvite(const std::string& inviteCode);

        /**
         * Delete (revoke) an invite.
         *
         * Requires the MANAGE_CHANNELS permission.
         *
         * \returns An invite object.
         */
        nlohmann::json revokeInvite(const std::string& inviteCode);

        /**
         * Accept an invite.
         *
         * This requires the guilds.join OAuth2 scope to be able to accept
         * invites on behalf of normal users (via an OAuth2 Bearer token). Bot
         * users are disallowed.
         *
         * \returns An invite object.
         */
        nlohmann::json acceptInvite(const std::string& inviteCode);

        /**
         * Get a list of invite objects (with invite metadata) for the channel.
         *
         * Requires the 'MANAGE_CHANNELS' permission.
         */
        nlohmann::json getChannelInvites(Snowflake channelId);

        /**
         * Create new invite for a channel.
         *
         * Requires the CREATE_INSTANT_INVITE permission.
         *
         * By default invite link is usable for 24 hours (86400 seconds), you
         * can pass maxAgeSecs parameter to override this. Pass 0 to create
         * invite with unlimited duration (it can be revoked using
         * \ref revokeInvite). You can also limit times invite can be used by
         * passing maxUses parameter.
         *
         * You can also create invite link which give temporary membership.
         * User will be removed from guild when he will go offline and don't
         * got a role.
         *
         * By default this method tries to reuse similar invite if avaliable,
         * you can override this by passing unique = true.
         */
        nlohmann::json createInvite(Snowflake channelId, unsigned maxAgeSecs = 86400,
                                    unsigned maxUses = 0,
                                    bool temporaryMembership = false,
                                    bool unique = false);

        /// @} REST_invites

        /// \defgroup REST_webhooks Webhook operations
        /// @{

        /**
         * Create new webhook.
         *
         * Requires MANAGE_WEBHOOKS permission.
         *
         * name is limited to 2-32 characters.
         * avatar must be 128x128.
         */
        nlohmann::json createWebhook(Snowflake channelId, const std::string& name, const boost::optional<Image>& avatar);

        /**
         * Get a webhook by it's ID.
         */
        nlohmann::json getWebhook(Snowflake id);

        /**
         * Get webhhoks for specified channel.
         */
        nlohmann::json getChannelWebhooks(Snowflake channelId);

        /**
         * Get webhhoks for specified guild.
         */
        nlohmann::json getGuildWebhooks(Snowflake guildId);

        /**
         * Change webhook name, name is limited to 2-3 characters.
         */
        nlohmann::json setWebhookName(Snowflake id, const std::string& newName);

        /**
         * Change webhook avatar, image must be 128x128.
         */
        nlohmann::json setWebhookAvatar(Snowflake id, const Image& newAvatar);

        /**
         * Delete webhook by it's id.
         *
         * User must be owner or must have MANAGE_WEBHOOKS permission [TODO: check].
         */
        void deleteWebhook(Snowflake id);

        /// @} REST_webhooks

        /// @} REST


#ifdef HEXICORD_RATELIMIT_PREDICTION
        RatelimitLock ratelimitLock;
#endif

        /**
         * Used authorization token.
         */
        const std::string token;
private:
        static constexpr const char* restBasePath = "/api/v6";

        void prepareRequestBody(REST::HTTPRequest& request,
                                const nlohmann::json& payload,
                                const std::vector<REST::MultipartEntity>& elements);

        // Throws RESTError or inherited class.
        void throwRestError(const REST::HTTPResponse& response, const nlohmann::json& payload);

#ifdef HEXICORD_RATELIMIT_PREDICTION
        void updateRatelimitsIfPresent(const std::string& endpoint, const std::unordered_map<std::string, std::string>& headers);
#endif

        static inline REST::MultipartEntity fileToMultipartEntity(const File& file);

        // We have to use std::shared_ptr instead of std::unique_ptr because
        // latter requires complete type but we forward-declare REST::HTTPSConnection.
        std::shared_ptr<REST::HTTPSConnection> restConnection;
        boost::asio::io_service& ioService; // non-owning reference to I/O service.
    };
} // namespace Hexicord

#endif // HEXICORD_REST_CLIENT_HPP
