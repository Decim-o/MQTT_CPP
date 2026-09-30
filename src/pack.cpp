#include <iostream>

#include <arpa/inet.h>
#include <cstring>

#include "pack.hpp"

namespace MQTT {

// Advance the caller's cursor so successive reads consume adjacent fields.
std::uint8_t unpack_u8(const std::uint8_t*& buffer){
    std::uint8_t value {*buffer};
    ++buffer;
    return value;
}

std::uint16_t unpack_u16(const std::uint8_t*& buffer){
    std::uint16_t value {};
    // memcpy permits reading fields that are not aligned as native integers.
    std::memcpy(&value, buffer, sizeof(uint16_t));
    buffer += sizeof(std::uint16_t);
    // Wire integers are big-endian regardless of the host byte order.
    return ntohs(value);
}

std::uint32_t unpack_u32(const std::uint8_t*& buffer){
    std::uint32_t value {};
    std::memcpy(&value, buffer, sizeof(std::uint32_t));
    buffer += sizeof(std::uint32_t);
    return ntohl(value);
}

std::vector<std::uint8_t>
unpack_bytes(const std::uint8_t*& buffer, std::size_t byte_count){
    std::vector<std::uint8_t> data (buffer, buffer + byte_count);
    buffer += byte_count;
    return data;
}

std::string unpack_string16(const std::uint8_t*& buffer){
    std::uint16_t string_length = unpack_u16(buffer);

    // The explicit length preserves embedded zero bytes without scanning past the field.
    std::string result(
            reinterpret_cast<const char*>(buffer),
            string_length
    );

    buffer += string_length;
    
    return result;
}

// Caller provides writable space; advancing buffer supports consecutive writes.
void pack_u8(std::uint8_t*& buffer, std::uint8_t value){
    // Packing overwrites the destination, so its previous contents do not matter.

    *buffer = value;
    buffer += sizeof(value);
}

void pack_u16(std::uint8_t*& buffer, std::uint16_t value){
    // Convert before copying so the wire representation is always big-endian.

    std::uint16_t network_value = htons(value);

    std::memcpy(buffer, &network_value, sizeof(value));
    buffer += sizeof(value);

}

void pack_u32(std::uint8_t*& buffer, std::uint32_t value){
    std::uint32_t network_value = htonl(value);
    std::memcpy(buffer, &network_value, sizeof(value));
    buffer += sizeof(value);
}

void pack_bytes(
        std::uint8_t*& buffer,
        const std::vector<std::uint8_t>& data)
{
    std::memcpy(buffer, data.data(), data.size());
    buffer += data.size();


}

} // namespace MQTT
