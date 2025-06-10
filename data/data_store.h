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

// Utility functions required for data store operations
bool entry_eq(HNode *lhs, HNode *rhs);

// The main request dispatcher
void do_request(std::vector<std::string> &cmd, Response &out);

#endif