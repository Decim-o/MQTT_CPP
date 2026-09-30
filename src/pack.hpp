#ifndef PACK_HPP
#define PACK_HPP

#include <cstdint>
#include <vector>
#include <string>


namespace MQTT{
// Reads advance buffer by reference so the caller can read the next field.
// Caller must ensure enough bytes are available; these helpers do not check bounds.
// bytes -> uint8_t
std::uint8_t unpack_u8(const std::uint8_t*& buffer);

// bytes -> uint16_t
std::uint16_t unpack_u16(const std::uint8_t*& buffer);

// bytes -> uint32_t
std::uint32_t unpack_u32(const std::uint8_t*& buffer);

// read a defined len of bytes
std::vector<std::uint8_t> 
unpack_bytes(const std::uint8_t*& buffer, std::size_t byte_count);

// unpack a string prefixed by its length as a uint16 value
std::string unpack_string16(const std::uint8_t*& buffer);

// Writes advance buffer and require enough writable space from the caller.
// append a uint8_t -> bytes into the bytestring
void pack_u8(std::uint8_t*& buffer, std::uint8_t value);

// append a uint16_t -> bytes into the bytestring
void pack_u16(std::uint8_t*& buffer, std::uint16_t value);

// append a uint32_t -> bytes into the bytestring
void pack_u32(std::uint8_t*& buffer, std::uint32_t value);

// Copy the full vector so embedded zero bytes are preserved.
void pack_bytes(
    std::uint8_t*& buffer, 
    const std::vector<std::uint8_t>& data
);

}

#endif
