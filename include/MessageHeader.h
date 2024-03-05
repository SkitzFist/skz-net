#ifndef SKZ_NET_MESSAGE_H_
#define SKZ_NET_MESSAGE_H_

#include <cstring>
#include <stdint.h>

namespace SkzNet {
constexpr const size_t MAX_STRING_SIZE = 100;

// size of header is 8 bytes
struct MessageHeader {
    size_t size;
    uint8_t type;
};

void writeHeader(char *data, MessageHeader &header) {
    memcpy(data, &header.size, sizeof(header.size));
    memcpy(data + sizeof(header.size), &header.type, sizeof(header.type));
}

void readHeader(char *data, MessageHeader &header) {
    memcpy(&header.size, data, sizeof(header.size));
    memcpy(&header.type, data + sizeof(header.size), sizeof(header.size));
}

} // namespace SkzNet

#endif
