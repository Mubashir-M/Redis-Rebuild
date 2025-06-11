#ifndef DATA_STORE_H
#define DATA_STORE_H

#include "server_common.h"
#include "hashmap.h"

#include <map>
#include <string>

extern HMap g_data;

struct Entry {
    struct HNode node;
    std::string key;
    std::string val;
};

enum {
    ERR_UNKNOWN = 1, // unknown command
    ERR_TOO_BIG = 2, // response too big
};

enum {
    TAG_NIL = 0,    // Represents a null value (e.g., key not found)
    TAG_ERR = 1,    // Represents an error (contains an error code + message)
    TAG_STR = 2,    // Represents a string (arbitrary binary data)
    TAG_INT = 3,    // Represents a 64-bit integer
    TAG_DBL = 4,    // Represents a double-precision floating-point number
    TAG_ARR = 5,    // Represents an array (can contain elements of any other type, including nested arrays)
};

void out_err(Buffer &out, uint32_t code, const std::string &msg);

// Utility functions required for data store operations
bool entry_eq(HNode *lhs, HNode *rhs);

// The main request dispatcher
void do_request(std::vector<std::string> &cmd, Buffer &out);

#endif