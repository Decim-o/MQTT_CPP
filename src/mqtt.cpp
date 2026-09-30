#include <cstdlib>
#include <array>
#include <utility>

#include "mqtt.hpp"
#include "pack.hpp"


namespace{

constexpr std::size_t MAX_LEN_BYTES {4};
constexpr std::size_t MAX_REMAINING_LENGTH {268'435'455};

std::size_t unpack_mqtt_connect(
        const std::uint8_t* buffer,
        const MQTT::mqtt_header& header,
        MQTT::mqtt_packet& packet);

std::size_t unpack_mqtt_publish(
        const std::uint8_t* buffer,
        const MQTT::mqtt_header& header,
        MQTT::mqtt_packet& packet);

std::size_t unpack_mqtt_subscribe(
        const std::uint8_t* buffer,
        const MQTT::mqtt_header& header,
        MQTT::mqtt_packet& packet);

std::size_t unpack_mqtt_unsubscribe(
        const std::uint8_t* buffer,
        const MQTT::mqtt_header& header,
        MQTT::mqtt_packet& packet);

std::size_t unpack_mqtt_ack(
        const std::uint8_t* buffer,
        const MQTT::mqtt_header& header,
        MQTT::mqtt_packet& packet);



std::uint8_t* pack_mqtt_header(const MQTT::mqtt_header& header);
std::uint8_t* pack_mqtt_ack(const MQTT::mqtt_packet& packet);
std::uint8_t* pack_mqtt_connack(const MQTT::mqtt_packet& packet);
std::uint8_t* pack_mqtt_suback(const MQTT::mqtt_packet& packet);
std::uint8_t* pack_mqtt_publish(const MQTT::mqtt_packet& packet);

/*
 * A shared signature lets the dispatcher call any unpacker through this table.
 */
using mqtt_unpack_handler = std::size_t (*)(
    const std::uint8_t*,
    const MQTT::mqtt_header&,
    MQTT::mqtt_packet&
);


/*
 * Packet type values match table indices, so dispatch needs no search.
 * Null entries represent unsupported types or packets handled directly.
 */
constexpr std::array<mqtt_unpack_handler, 15>
unpack_handlers {
    nullptr,                  // 0  RESERVED
    unpack_mqtt_connect,      // 1  CONNECT
    nullptr,                  // 2  CONNACK
    unpack_mqtt_publish,      // 3  PUBLISH
    unpack_mqtt_ack,          // 4  PUBACK
    unpack_mqtt_ack,          // 5  PUBREC
    unpack_mqtt_ack,          // 6  PUBREL
    unpack_mqtt_ack,          // 7  PUBCOMP
    unpack_mqtt_subscribe,    // 8  SUBSCRIBE
    nullptr,                  // 9  SUBACK
    unpack_mqtt_unsubscribe,  // 10 UNSUBSCRIBE
    nullptr,                  // 11 UNSUBACK
    nullptr,                  // 12 PINGREQ
    nullptr,                  // 13 PINGRESP
    nullptr                   // 14 DISCONNECT
};

}

namespace MQTT {

[[nodiscard]]
int mqtt_encode_length
(std::vector<std::uint8_t>& output, std::size_t remaining_length)
{
    int byte_count {};

    // Reject oversized values before appending a partial encoding.
    if(remaining_length > MAX_REMAINING_LENGTH){
        return 0;
    }
    do{

        std::uint8_t digit =
            static_cast<std::uint8_t>(remaining_length % 128);

        remaining_length /= 128;

        if(remaining_length > 0){
            // The high bit tells the decoder that another length byte follows.
            digit |= 0x80;
        }
        output.push_back(digit);
        ++byte_count;
    } while(remaining_length > 0);

    return byte_count;
}

std::uint64_t mqtt_decode_length(const std::uint8_t*& buffer){
    std::uint8_t byte {};
    std::uint32_t multiplier {1};
    std::uint64_t value {};
    std::size_t byte_count {};

    do{
        if(byte_count >= MAX_LEN_BYTES){
            return 0;
        }

        // Only the low seven bits contribute to this base-128 digit.
        byte = *buffer++;
        value += static_cast<std::uint32_t>(byte & 0x7F) * multiplier;
        ++byte_count;

        // Each following digit has 128 times the weight of the previous one.
        if((byte & 0x80) != 0){
            multiplier *= 128;
        }
    } while((byte & 0x80) != 0);

    return value;
}

int unpack_mqtt_packet(
    const std::uint8_t* buffer,
    mqtt_packet& packet)
{
    if (buffer == nullptr) {
        return 0;
    }

    /*
     * The first byte selects the unpacker and carries flags needed by it.
     */
    mqtt_header header {};
    header.byte = *buffer;

    /*
     * Unpackers expect to start at Remaining Length, after the type byte.
     */
    ++buffer;

    const auto type = header.type();

    /*
     * These packet types have no variable header or payload to unpack.
     */
    if (type == packet_type::DISCONNECT ||
        type == packet_type::PINGREQ ||
        type == packet_type::PINGRESP)
    {
        packet = header;
        return 0;
    }

    const std::size_t index =
        static_cast<std::size_t>(
            type
        );

    /*
     * A received type can exceed the table, so check before indexing it.
     */
    if (index >= unpack_handlers.size()) {
        return 0;
    }

    const auto handler =
        unpack_handlers[index];

    /*
     * Unsupported entries must not be called as function pointers.
     */
    if (handler == nullptr) {
        return 0;
    }

    return static_cast<int>(
        handler(
            buffer,
            header,
            packet
        )
    );
}

} // namespace MQTT

namespace {

std::size_t unpack_mqtt_connect(const std::uint8_t* buffer,
                       const MQTT::mqtt_header& header,
                       MQTT::mqtt_packet& packet)
{
    MQTT::mqtt_connect connect {};
    connect.header = header;

    const std::size_t remaining_length
        = static_cast<std::size_t>(MQTT::mqtt_decode_length(buffer));

    // Assume MQTT 3.1.1: the name length, name, and level occupy seven bytes.
    // Advance from the decoded body start because Remaining Length varies in size.
    buffer += 7;

    connect.flags.byte = MQTT::unpack_u8(buffer);

    connect.payload.keepalive = MQTT::unpack_u16(buffer);

    connect.payload.client_id = MQTT::unpack_string16(buffer);

    // Optional fields appear only when their flags are set; reading absent
    // fields would shift the cursor into the next field or packet.
    if(connect.flags.will()){
        connect.payload.will_topic = MQTT::unpack_string16(buffer);

        const auto will_message_length = MQTT::unpack_u16(buffer);
        connect.payload.will_message = MQTT::unpack_bytes(buffer, will_message_length);
    }

    if(connect.flags.username()){
        connect.payload.username = MQTT::unpack_string16(buffer);
    }

    if(connect.flags.password()){
        connect.payload.password = MQTT::unpack_string16(buffer);
    }

    packet = std::move(connect);

    return remaining_length;
}

std::size_t unpack_mqtt_publish(
        const std::uint8_t* buffer,
        const MQTT::mqtt_header& header,
        MQTT::mqtt_packet& packet)
{
    MQTT::mqtt_publish publish {};
    publish.header = header;

    // Decoding advances buffer past the variable-size Remaining Length field.

    const auto remaining_length = static_cast<std::size_t>(
            MQTT::mqtt_decode_length(buffer)
        );

    const std::uint8_t* variable_header_start = buffer;

    publish.topic = MQTT::unpack_string16(buffer);

    /* QoS 1 and QoS 2 PUBLISH packets contain a
       packet identifier

       QoS 0 packets do not.
    */

    if(publish.header.qos() != MQTT::qos_level::AT_MOST_ONCE){
        publish.packet_id = MQTT::unpack_u16(buffer);
    }

    /*
     * Number of bytes consumed by:
     *   2-byte topic length
     * + topic
     * + optional 2-byte packet identifier
     */

    const auto variable_header_length =
        static_cast<std::size_t>(
                buffer - variable_header_start
        );

    // Guard the subtraction so a malformed length cannot wrap size_t.

    if(variable_header_length > remaining_length){
        return 0;
    }

    const std::size_t payload_length = remaining_length - variable_header_length;

    // Payload is binary; its length comes from the packet, not a null terminator.
    publish.payload =
        MQTT::unpack_bytes(buffer, payload_length);

    packet = std::move(publish);

    return remaining_length;

}

std::size_t unpack_mqtt_subscribe(
    const std::uint8_t* buffer,
    const MQTT::mqtt_header& header,
    MQTT::mqtt_packet& packet)
{
    MQTT::mqtt_subscribe subscribe {};
    subscribe.header = header;

    const auto remaining_length = static_cast<std::size_t>(
        MQTT::mqtt_decode_length(buffer)
    );

    std::size_t remaining_bytes = remaining_length;

    if (remaining_bytes < sizeof(std::uint16_t)) {
        return 0;
    }

    subscribe.packet_id = MQTT::unpack_u16(buffer);
    remaining_bytes -= sizeof(std::uint16_t);

    while (remaining_bytes > 0) {
        // A SUBSCRIBE entry needs at minimum:
        // 2-byte topic length + 1-byte QoS.
        if (remaining_bytes <
            sizeof(std::uint16_t) + sizeof(std::uint8_t))
        {
            return 0;
        }

        const std::uint8_t* topic_start = buffer;

        std::string topic = MQTT::unpack_string16(buffer);

        // Count the length prefix too, so the next tuple starts at the right byte.
        const auto topic_field_length =
            static_cast<std::size_t>(buffer - topic_start);

        if (topic_field_length > remaining_bytes) {
            return 0;
        }

        remaining_bytes -= topic_field_length;

        if (remaining_bytes < sizeof(std::uint8_t)) {
            return 0;
        }

        const auto qos_byte = MQTT::unpack_u8(buffer);
        remaining_bytes -= sizeof(std::uint8_t);

        if (qos_byte >
            static_cast<std::uint8_t>(
                MQTT::qos_level::EXACTLY_ONCE))
        {
            return 0;
        }

        MQTT::mqtt_subscribe_tuple tuple {
            .topic = std::move(topic),
            .qos = static_cast<MQTT::qos_level>(qos_byte)
        };

        subscribe.tuples.push_back(std::move(tuple));
    }

    packet = std::move(subscribe);

    return remaining_length;
}

std::size_t unpack_mqtt_unsubscribe(
    const std::uint8_t* buffer,
    const MQTT::mqtt_header& header,
    MQTT::mqtt_packet& packet)
{
    MQTT::mqtt_unsubscribe unsubscribe {};
    unsubscribe.header = header;

    const auto remaining_length = static_cast<std::size_t>(
        MQTT::mqtt_decode_length(buffer)
    );

    std::size_t remaining_bytes = remaining_length;

    if (remaining_bytes < sizeof(std::uint16_t)) {
        return 0;
    }

    unsubscribe.packet_id = MQTT::unpack_u16(buffer);
    remaining_bytes -= sizeof(std::uint16_t);

    while (remaining_bytes > 0) {
        if (remaining_bytes < sizeof(std::uint16_t)) {
            return 0;
        }

        const std::uint8_t* topic_start = buffer;

        std::string topic = MQTT::unpack_string16(buffer);

        const auto topic_field_length =
            static_cast<std::size_t>(buffer - topic_start);

        if (topic_field_length > remaining_bytes) {
            return 0;
        }

        remaining_bytes -= topic_field_length;

        unsubscribe.tuples.push_back(std::move(topic));
    }

    packet = std::move(unsubscribe);

    return remaining_length;
}

std::size_t unpack_mqtt_ack(
    const std::uint8_t* buffer,
    const MQTT::mqtt_header& header,
    MQTT::mqtt_packet& packet)
{
    MQTT::mqtt_ack ack {};
    ack.header = header;

    const auto remaining_length = static_cast<std::size_t>(
        MQTT::mqtt_decode_length(buffer)
    );

    /*
     * These ACK types need a packet identifier to match the acknowledged message.
     */
    if (remaining_length < sizeof(std::uint16_t)) {
        return 0;
    }

    ack.packet_id = MQTT::unpack_u16(buffer);

    packet = ack;

    return remaining_length;
}



} // anonymous namespace
