#ifndef HEXICORD_TYPES_SNOWFLAKE_HPP
#define HEXICORD_TYPES_SNOWFLAKE_HPP

#include <cstdint>                     // uint64_t
#include <string>                      // std::string
#include <vector>                      // std::vector
#include <functional>                  // std::hash
#include "hexicord/json.hpp"           // nlohmann::json

namespace Hexicord {
    struct Snowflake {
        constexpr Snowflake() : value(0) {}
        constexpr Snowflake(uint64_t value) : value(value) {}
        explicit Snowflake(const std::string& strvalue) : value(std::stoull(strvalue)) {}
        explicit Snowflake(const char* strvalue) : value(std::stoull(strvalue)) {}

        struct Parts {
            unsigned counter       : 12;
            unsigned processId     : 5;
            unsigned workerId      : 5;
            uint64_t timestamp     : 42;
        };

        union {
            uint64_t value;
            Parts parts;
        };

        static constexpr uint64_t discordEpochMs = 1420070400000;

        inline constexpr time_t unixTimestamp() const {
            return this->parts.timestamp + discordEpochMs / 1000;
        }

        inline constexpr unsigned long long unixTimestampMs() const {
            return this->parts.timestamp + discordEpochMs;
        }

        inline constexpr operator uint64_t() const { return value; }
    };

    inline void to_json(nlohmann::json& json, const Snowflake& snowflake) {
        json = uint64_t(snowflake);
    }

    inline void from_json(const nlohmann::json& json, Snowflake& snowflake) {
        snowflake.value = json.get<uint64_t>();
    }
} // namespace Hexicord

namespace std {
    template<>
    struct hash<Hexicord::Snowflake> {
    public:
        constexpr inline size_t operator()(const Hexicord::Snowflake& snowflake) const {
            return static_cast<uint64_t>(snowflake);
        }
    };
}

#endif // HEXICORD_TYPES_SNOWFLAKE_HPP
