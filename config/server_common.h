#ifndef SERVER_COMMON_H
#define SERVER_COMMON_H

#include "DList.h"

#include <vector>

struct Conn {
    int fd = -1;

    bool want_read = false;
    bool want_write = false;
    bool want_close = false;

    std::vector<uint8_t> incoming;
    std::vector<uint8_t> outgoing;

    // timer
    uint64_t last_active_ms = 0;
    DList idle_node;
};

enum {
    RES_OK = 0, // Operation successful
    RES_ERR = 1, // Generic Error or Unregonized command
    RES_NX = 2, // Key not found (for GET operation)
};

typedef std::vector<uint8_t> Buffer;

#endif