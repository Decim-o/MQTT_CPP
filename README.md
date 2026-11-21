# MQTT

A C++20 static library for working with MQTT 3.1.1 packets and POSIX stream sockets. It provides packet encoding and decoding for a subset of MQTT control packets, along with helpers for creating TCP and Unix-domain listeners and accepting connections. Applications supply their own event loop, packet framing, and protocol state.

## Build and test

Use a C++20 compiler and CMake 3.20 or newer on a POSIX system. Run these commands from the repository root:

```sh
cmake -S src -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

The build produces the `mqtt` static library. The `network` test uses IPv4 loopback and an ephemeral port to verify nonblocking listeners, nonblocking accepted sockets, `TCP_NODELAY`, and the behavior of `accept_connection` when no client is waiting. Configure with `-DBUILD_TESTING=OFF` for a library-only build.

## Use the library

When this repository is available in your CMake project at `vendor/mqtt`, add its `src` directory and link the `mqtt` target:

```cmake
add_subdirectory(vendor/mqtt/src)
target_link_libraries(my_app PRIVATE mqtt)
target_include_directories(my_app PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/vendor/mqtt/src")
```

For example, encode a QoS 0 PUBLISH packet containing the payload `OK`:

```cpp
#include "mqtt.hpp"

int main() {
    const MQTT::mqtt_packet packet = MQTT::mqtt_packet_publish(
        MQTT::PUBLISH_BYTE, 0, "status", {0x4f, 0x4b});
    const auto bytes = MQTT::pack_mqtt_packet(packet, MQTT::packet_type::PUBLISH);
}
```

## Packet support

| Operation | Control packets |
| --- | --- |
| Encode and decode | PUBLISH, PUBACK, PUBREC, PUBREL, PUBCOMP, PINGREQ, PINGRESP, DISCONNECT |
| Encode | CONNACK, SUBACK, UNSUBACK |
| Decode | CONNECT, SUBSCRIBE, UNSUBSCRIBE |

The decoder accepts a raw pointer and requires the caller to supply a complete packet and validate buffer bounds before decoding. Its return value is the MQTT Remaining Length field. Zero can indicate a header-only packet, an unsupported type, or a decoding error.

## Source layout

- [`src/mqtt.hpp`](src/mqtt.hpp) defines packet types and the encoding and decoding API.
- [`src/pack.hpp`](src/pack.hpp) contains the declarations for byte and integer serialization helpers.
- [`src/network.hpp`](src/network.hpp) declares the socket helpers and `Network::FileD`, a move-only file descriptor owner.
- [`src/tests/test_network.cpp`](src/tests/test_network.cpp) exercises the TCP listener and accept helpers.
