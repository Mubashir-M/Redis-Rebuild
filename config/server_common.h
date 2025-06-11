#ifndef SERVER_COMMON_H
#define SERVER_COMMON_H

#include <vector>

struct Conn {
    int fd = -1;

    bool want_read = false;
    bool want_write = false;
    bool want_close = false;

    std::vector<uint8_t> incoming;
    std::vector<uint8_t> outgoing;
};

enum {
    RES_OK = 0, // Operation successful
    RES_ERR = 1, // Generic Error or Unregonized command
    RES_NX = 2, // Key not found (for GET operation)
};

typedef std::vector<uint8_t> Buffer;

#endif