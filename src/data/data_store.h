#ifndef DATA_STORE_H
#define DATA_STORE_H

#include "server_common.h"
#include "hashmap.h"
#include "zset.h"
#include "heap.h"
#include "thread_pool.h"

#include <map>
#include <string>

struct GlobalData {
    HMap db;
    // a map of all client connections, keyed by fd
    std::vector<Conn *> fd2conn;
    // timers for idle connections
    DList idle_list;
    // timers for TTLs
    std::vector<HeapItem> heap;
    // the thread pool
    ThreadPool thread_pool;
    
};

enum {
    T_INIT = 0,
    T_STR = 1,
    T_ZSET = 2,
};

struct Entry {
    struct HNode node;
    std::string key;
    // value
    uint32_t type = 0;
    // one of the following
    union {
        std::string str;
        ZSet zset;
    };

    // for TTL
    size_t heap_idx = -1;
    //

    explicit Entry(uint32_t type): type(type){
        if(type == T_STR){
            new (&str) std::string;
        } else if (type == T_ZSET){
            new (&zset) ZSet;
        }
    }

    ~Entry(){
        if(type == T_STR){
            str.~basic_string();
        } else if(type == T_ZSET){
            zset_clear(&zset);
        }
    }
};

struct LookupKey {
    struct HNode node; // hashtable node
    std::string key;
};

// error code for TAG_ERR
enum {
    ERR_UNKNOWN = 1,    // unknown command
    ERR_TOO_BIG = 2,    // response too big
    ERR_BAD_TYP = 3,    // unexpected value type
    ERR_BAD_ARG = 4,    // bad arguments
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
uint64_t str_hash(const uint8_t *data, size_t len);
// Utility functions required for data store operations
bool entry_eq(HNode *lhs, HNode *rhs);

void entry_del(Entry *ent);
void entry_set_ttl(Entry *ent, int64_t ttl_ms);

// The main request dispatcher
void do_request(std::vector<std::string> &cmd, Buffer &out);

extern GlobalData g_data;
#endif