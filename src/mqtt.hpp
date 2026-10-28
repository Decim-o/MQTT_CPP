#ifndef MQTT_HPP
#define MQTT_HPP

#include <iostream>
#include <cstddef>
#include <cstdint>
#include <vector>
#include <string>
#include <variant>
#include <optional>
#include <span>


namespace MQTT
{
inline constexpr std::size_t MQTT_HEADER_LEN { 2 };
inline constexpr std::size_t MQTT_ACK_LEN    { 4 };

/*
   * Stub bytes, used for generic replies, these represent the first byte in fixed
     header
*/

inline constexpr std::uint8_t CONNACK_BYTE  { 0x20 };
inline constexpr std::uint8_t PUBLISH_BYTE  { 0x30 };
inline constexpr std::uint8_t PUBACK_BYTE   { 0x40 };
inline constexpr std::uint8_t PUBREC_BYTE   { 0x50 };
inline constexpr std::uint8_t PUBREL_BYTE   { 0x62 };
inline constexpr std::uint8_t PUBCOMP_BYTE  { 0x70 };
inline constexpr std::uint8_t SUBACK_BYTE   { 0x90 };
inline constexpr std::uint8_t UNSUBACK_BYTE { 0xB0 };
inline constexpr std::uint8_t PINGRESP_BYTE { 0xD0 };

// message types or control packet types
enum class packet_type : std::uint8_t {
    CONNECT     = 1, 
    CONNACK     = 2,
    PUBLISH     = 3,
    PUBACK      = 4,
    PUBREC      = 5,
    PUBREL      = 6,
    PUBCOMP     = 7,
    SUBSCRIBE   = 8,
    SUBACK      = 9,
    UNSUBSCRIBE = 10,
    UNSUBACK    = 11,
    PINGREQ     = 12,
    PINGRESP    = 13,
    DISCONNECT  = 14
};

enum class qos_level : std::uint8_t { 
    AT_MOST_ONCE  = 0, 
    AT_LEAST_ONCE = 1, 
    EXACTLY_ONCE  = 2
};


// Masks keep wire bit positions explicit rather than relying on bitfield layout.
struct mqtt_header{
    std::uint8_t byte {};

    [[nodiscard]] constexpr bool retain() const {
        return byte & 0b0000'0001;
    }

    [[nodiscard]] constexpr qos_level qos() const noexcept {
        return static_cast<qos_level>(
                (byte >> 1) & 0b0000'0011
        );
    }

    [[nodiscard]] constexpr bool dup() const {
        return ((byte >> 3) & 0b0000'0001) != 0;
    }

    [[nodiscard]] constexpr packet_type type() const noexcept{
        return static_cast<packet_type>(
                (byte >> 4) & 0b0000'1111
        );
    }
};

// These flags determine which optional fields the CONNECT payload contains.
struct mqtt_connect_flags{
    std::uint8_t byte {};

    [[nodiscard]] constexpr bool reserved() const noexcept {
        return (byte & 0b0000'0001) != 0;
    }

    [[nodiscard]] constexpr bool clean_session() const noexcept {
        return (byte & 0b0000'0010) != 0;
    }

    [[nodiscard]] constexpr bool will() const noexcept {
        return (byte & 0b0000'0100) != 0;
    }

    [[nodiscard]] constexpr qos_level will_qos() const noexcept {
        return static_cast<qos_level>(
                (byte >> 3) & 0b0000'0011
        );
    }

    [[nodiscard]] constexpr bool will_retain() const noexcept {
        return ((byte >> 5) & 0b0000'0001) != 0;
    }

    [[nodiscard]] constexpr bool password() const noexcept {
        return ((byte >> 6) & 0b0000'0001) != 0;
    }

    [[nodiscard]] constexpr bool username() const noexcept {
        return ((byte >> 7) & 0b0000'0001) != 0;
    }
    
};

struct mqtt_connect_payload{
    std::uint16_t keepalive {};

    std::string client_id {};
    std::string username {};
    std::string password {};
    std::string will_topic {};
    std::vector<std::uint8_t> will_message {};
};

struct mqtt_connect{
    mqtt_header header {};
    mqtt_connect_flags flags {};
    mqtt_connect_payload payload {};
};

struct mqtt_connack{
    mqtt_header header {};
    std::uint8_t byte {};
    std::uint8_t return_code {};

    [[nodiscard]] constexpr bool session_present() const noexcept {
        return byte & 0b0000'0001;
    }

    [[nodiscard]] constexpr std::uint8_t reserved() const noexcept {
        return (byte >> 1) & 0b0111'1111;
        
    }
};

struct mqtt_subscribe_tuple {
    std::string topic {};
    qos_level qos {};
};

struct mqtt_subscribe{
    mqtt_header header {};
    std::uint16_t packet_id {};
    std::vector<mqtt_subscribe_tuple> tuples {};
};


struct mqtt_unsubscribe{
    mqtt_header header {};
    std::uint16_t packet_id {};
    std::vector<std::string> tuples {};
};

struct mqtt_suback{
    mqtt_header header {};

    std::uint16_t packet_id {};
    std::vector<std::uint8_t> return_codes {};
};

struct mqtt_publish{
    mqtt_header header {};

    std::uint16_t packet_id {};
    std::string topic {};
    std::vector <std::uint8_t> payload {};
};

struct mqtt_ack{
    mqtt_header header {};
    std::uint16_t packet_id {};
};

using mqtt_puback   = mqtt_ack;
using mqtt_pubrec   = mqtt_ack;
using mqtt_pubrel   = mqtt_ack;
using mqtt_pubcomp  = mqtt_ack;
using mqtt_unsuback = mqtt_ack;

using mqtt_pingreq    = mqtt_header;
using mqtt_pingresp   = mqtt_header;
using mqtt_disconnect = mqtt_header;

using mqtt_packet = std::variant<
    mqtt_ack,
    mqtt_connect,
    mqtt_connack,
    mqtt_suback,
    mqtt_publish,
    mqtt_subscribe,
    mqtt_unsubscribe,
    mqtt_header
>;


[[nodiscard]]
int mqtt_encode_length(std::vector<std::uint8_t>& output,
                       std::size_t remaining_length
        );

// Advances buffer so the next read starts after the encoded length.
[[nodiscard]] 
std::uint64_t mqtt_decode_length(const std::uint8_t*& buffer);

// Caller supplies a complete packet: this pointer-only API cannot check buffer size.
// Returns Remaining Length; zero also represents unsupported types or errors.
[[nodiscard]]
int unpack_mqtt_packet(const std::uint8_t* buffer,
                       mqtt_packet& packet);

std::vector<std::uint8_t>
pack_mqtt_packet(const mqtt_packet& packet,
                 packet_type type);
[[nodiscard]]
std::vector<std::uint8_t> pack_mqtt_packet(const mqtt_packet& packet, 
                                           packet_type type
        );

[[nodiscard]] 
constexpr mqtt_header mqtt_packet_header(std::uint8_t byte){
    mqtt_header header {};
    header.byte = byte;
    return header;
    
};

[[nodiscard]]
mqtt_ack mqtt_packet_ack(std::uint8_t header_byte, std::uint16_t packet_id);

[[nodiscard]]
mqtt_connack mqtt_packet_connack(std::uint8_t header_byte,
                                 std::uint8_t flags_byte,
                                 std::uint8_t return_code
);

[[nodiscard]]
mqtt_suback mqtt_packet_suback(std::uint8_t header_byte,
                               std::uint16_t packet_id,
                               std::vector<std::uint8_t> return_codes
);

[[nodiscard]]
mqtt_publish mqtt_packet_publish(std::uint8_t header_byte,
                                 std::uint16_t packet_id,
                                 std::string topic,
                                 std::vector<std::uint8_t> payload
);


}

#endif
