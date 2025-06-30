#include "data_store.h"
#include "server_config.h"
#include "buffer_operations.h"
#include "assert.h"
#include "zset.h"
#include "utils/timer.h"


GlobalData g_data;

bool entry_eq(HNode *lhs, HNode *rhs){
    struct Entry *le = container_of(lhs, struct Entry, node);
    struct Entry *re = container_of(rhs, struct Entry, node);
    return le->key == re->key;
};

uint64_t str_hash(const uint8_t *data, size_t len){
    uint32_t h = 0x811C9DC5;
    for (size_t i = 0; i< len; i++){
        h = (h + data[i]) * 0x01000193;
    }
    return h;
};

void entry_set_ttl(Entry *ent, int64_t ttl_ms){
    if(ttl_ms < 0 && ent->heap_idx != (size_t)-1){
        // setting a negative TTL means removing TTL
        heap_delete(g_data.heap, ent->heap_idx);
        ent->heap_idx = -1;
    } else if (ttl_ms >= 0){
        // add or update the heap data structure
        uint64_t expire_at = get_monotonic_msec() + (uint64_t)ttl_ms;
        HeapItem item = {expire_at, &ent->heap_idx};
        heap_upsert(g_data.heap, ent->heap_idx, item);
    }
};

// append serialzied data types to the back
static void out_nil(Buffer &out){
    buf_append_u8(out, TAG_NIL);
};

static void out_str(Buffer &out, const char *s, size_t size){
    buf_append_u8(out, TAG_STR);
    buf_append_u32(out, (uint32_t)size);
    buf_append(out, (const uint8_t *)s, size);
};

static void out_int(Buffer &out, int64_t val){
    buf_append_u8(out, TAG_INT);
    buf_append_i64(out, val);
};

static void out_arr(Buffer &out, uint32_t n){
    buf_append_u8(out, TAG_ARR);
    buf_append_u32(out, n);
};

static void out_dbl(Buffer &out, double val){
    buf_append_u8(out, TAG_DBL);
    buf_append_dbl(out, val);
};

static Entry *entry_new(uint32_t type){
    Entry *ent = new Entry(type);
    return ent;
};

static void entry_del_sync(Entry *ent){
    if(ent->type == T_ZSET){
        zset_clear(&ent->zset);
    }
    delete ent;
};
static void entry_del_func(void *arg){
    entry_del_sync((Entry *)arg);
};

void entry_del(Entry *ent){
    // unlink it from any data structures
    entry_set_ttl(ent, -1);
    // run the destructor in a thread pool for large data structures
    size_t set_size = (ent->type == T_ZSET) ? hm_size(&ent->zset.hmap) : 0;
    if(set_size > k_large_container_size){
        thread_pool_queue(&g_data.thread_pool, &entry_del_func, ent);
    } else {
        entry_del_sync(ent); // small;  avoid context swtiches
    }
};

void out_err(Buffer &out, uint32_t code, const std::string &msg){
    buf_append_u8(out, TAG_ERR);
    buf_append_u32(out, code);
    buf_append_u32(out, (uint32_t)msg.size());
    buf_append(out, (const uint8_t*)msg.data(), msg.size());
};

static void do_get(std::vector<std::string> &cmd, Buffer &out){
    // a dummy `Entry` just for the lookup
    LookupKey key;
    key.key.swap(cmd[1]);
    key.node.hcode = str_hash((uint8_t *)key.key.data(), key.key.size());
    // hashtable lookup
    HNode *node = hm_lookup(&g_data.db, &key.node, &entry_eq);
    if (!node) {
        return out_nil(out);
    }
   
    Entry *ent = container_of(node, Entry, node);
    return out_str(out, ent->str.data(), ent->str.size());
};

static void do_set(std::vector<std::string>& cmd, Buffer &out){
    LookupKey key;
    key.key.swap(cmd[1]);
    key.node.hcode = str_hash((uint8_t *)key.key.data(), key.key.size());

    HNode *node = hm_lookup(&g_data.db, &key.node, &entry_eq);

    if(node){
        // found, update the value
        Entry *ent = container_of(node, Entry, node);
        if(ent->type != T_STR){
            return out_err(out, ERR_BAD_TYP, "a non-string value exists");
        }
        ent->str.swap(cmd[2]);
    } else {
        // not found, allocate & insert a new pair
        Entry *ent = new Entry(T_STR);
        ent->key.swap(key.key);
        ent->node.hcode= key.node.hcode;
        ent->str.swap(cmd[2]);
        hm_insert(&g_data.db, &ent->node);
    }
    return out_nil(out);
};

static void do_del(std::vector<std::string> &cmd, Buffer &out){
    LookupKey key;
    key.key.swap(cmd[1]);

    key.node.hcode = str_hash((uint8_t *)key.key.data(), key.key.size());
    HNode *node = hm_delete(&g_data.db, &key.node, &entry_eq);
    if(node){
        entry_del(container_of(node, Entry, node));
    }
    return out_int(out, node ? 1 : 0);
};

static bool cb_keys(HNode *node, void *arg){
    Buffer &out = *(Buffer *)arg;
    const std::string &key = container_of(node, Entry, node)->key;
    out_str(out, key.data(), key.size());
    return true;
};

static void do_keys(std::vector<std::string> &, Buffer &out){
    out_arr(out, (uint32_t)hm_size(&g_data.db));
    hm_foreach(&g_data.db, &cb_keys, (void *)&out);
};


static size_t out_begin_arr(Buffer &out) {
    out.push_back(TAG_ARR);
    buf_append_u32(out, 0);     // filled by out_end_arr()
    return out.size() - 4;      // the `ctx` arg
}
static void out_end_arr(Buffer &out, size_t ctx, uint32_t n) {
    assert(out[ctx - 1] == TAG_ARR);
    memcpy(&out[ctx], &n, 4);
}

static bool str2dbl(const std::string &s, double &out){
    char *endp = NULL;
    out = strtod(s.c_str(), &endp);
    return endp == s.c_str() + s.size() && !isnan(out);
};

static bool str2int(const std::string &s, int64_t &out){
    char *endp = NULL;
    out = strtoll(s.c_str(), &endp, 10);
    return endp == s.c_str() + s.size();
};

static const ZSet k_empty_zset;
static ZSet *expect_zset(std::string &s){
    LookupKey key;
    key.key.swap(s);
    key.node.hcode = str_hash((uint8_t *)key.key.data(), key.key.size());
    HNode *hnode = hm_lookup(&g_data.db, &key.node, &entry_eq);

    if(!hnode){ // a non-existent key is treaded as an empty zset
        return (ZSet *)&k_empty_zset;
    }

    Entry *ent = container_of(hnode, Entry, node);
    return ent->type == T_ZSET ? &ent->zset : NULL;
};

static void do_zadd(std::vector<std::string> &cmd, Buffer &out){
    double score = 0;
    if(!str2dbl(cmd[2], score)){
        return out_err(out, ERR_BAD_ARG, "epect float");
    }

    LookupKey key;
    key.key.swap(cmd[1]);
    key.node.hcode = str_hash((uint8_t *)key.key.data(), key.key.size());

    HNode *hnode = hm_lookup(&g_data.db, &key.node, &entry_eq);

    Entry *ent = NULL;
    if(!hnode){
        ent = entry_new(T_ZSET);
        ent->key.swap(key.key);
        ent->node.hcode = key.node.hcode;
        hm_insert(&g_data.db, &ent->node);
    } else {
        ent = container_of(hnode, Entry, node);
        if(ent->type != T_ZSET){
            return out_err(out, ERR_BAD_TYP, "expect zset");
        }
    }

    const std::string &name = cmd[3];
    bool added = zset_insert(&ent->zset, name.data(), name.size(), score);

    return out_int(out, (int64_t)added);
};

static void do_zrem(std::vector<std::string> &cmd, Buffer &out){
    ZSet *zset = expect_zset(cmd[1]);
    if(!zset){
        return out_err(out, ERR_BAD_TYP, "expect zset");
    }

    const std::string &name  = cmd[2];
    ZNode *znode = zset_lookup(zset, name.data(), name.size());
    if(znode){
        zset_delete(zset, znode);
    }
    return out_int(out, znode ? 1 : 0);
};

static void do_zscore(std::vector<std::string> &cmd, Buffer &out){
    ZSet *zset = expect_zset(cmd[1]);
    if(!zset){
        return out_err(out, ERR_BAD_TYP, "ecpext zset");
    }

    const std::string &name = cmd[2];
    ZNode *znode = zset_lookup(zset, name.data(), name.size());
    return znode ? out_dbl(out, znode->score) : out_nil(out);
};

// zquery zset score name offset limit
static void do_zquery(std::vector<std::string> &cmd, Buffer &out){
    // parse args
    double score = 0;
    if(!str2dbl(cmd[2], score)){
        return out_err(out, ERR_BAD_ARG, "expect fp number");
    }

    const std::string &name = cmd[3];
    int64_t offset = 0, limit = 0;
    if(!str2int(cmd[4], offset) || !str2int(cmd[5], limit)){
        return out_err(out, ERR_BAD_ARG, "expext int");
    }

    // get the zset
    ZSet *zset = expect_zset(cmd[1]);
    if(!zset){
        return out_err(out, ERR_BAD_TYP, "expect zset");
    }

    // seek to the key
    if(limit <= 0){
        return out_arr(out, 0);
    }

    ZNode *znode = zset_seekge(zset, score, name.data(), name.size());
    znode = znode_offset(znode, offset);

    //output
    size_t ctx = out_begin_arr(out);
    int64_t n = 0;
    while (znode && n < limit) {
        out_str(out, znode->name, znode->len);
        out_dbl(out, znode->score);
        znode = znode_offset(znode, +1);
        n += 2;
    }
    out_end_arr(out, ctx, (uint32_t)n);
};

static void do_expire(std::vector<std::string> &cmd, Buffer &out){
    int64_t ttl_ms = 0;
    if(!str2int(cmd[2], ttl_ms)){
        return out_err(out, ERR_BAD_ARG, "expect int64");
    }

    LookupKey key;
    key.key.swap(cmd[1]);
    key.node.hcode = str_hash((uint8_t *)key.key.data(), key.key.size());

    HNode *node = hm_lookup(&g_data.db, &key.node, &entry_eq);

    if(node){
        Entry *ent = container_of(node, Entry, node);
        entry_set_ttl(ent, ttl_ms);
    } 
    return out_int(out, node ? 1 : 0);
};

static void do_ttl(std::vector<std::string> &cmd, Buffer &out){
    LookupKey key;
    key.key.swap(cmd[1]);
    key.node.hcode = str_hash((uint8_t *)key.key.data(), key.key.size());

    HNode *node = hm_lookup(&g_data.db, &key.node, &entry_eq);
    if(!node){
        out_int(out, -2); // not found
    }

    Entry *ent = container_of(node, Entry, node);
    if(ent->heap_idx == (size_t)-1){
        return out_int(out, -1); // not TTL
    }

    uint64_t expire_at = g_data.heap[ent->heap_idx].val;
    uint64_t now_ms = get_monotonic_msec();
    return out_int(out, expire_at > now_ms ? (expire_at - now_ms) : 0);
};

void do_request(std::vector<std::string> &cmd, Buffer &out) {
    if (cmd.size() == 2 && cmd[0] == "get") {
        return do_get(cmd, out);
    } else if (cmd.size() == 3 && cmd[0] == "set") {
        return do_set(cmd, out);
    } else if (cmd.size() == 2 && cmd[0] == "del") {
        return do_del(cmd, out);
    } else if (cmd.size() == 3 && cmd[0] == "pexpire") {
        return do_expire(cmd, out);
    } else if (cmd.size() == 2 && cmd[0] == "pttl") {
        return do_ttl(cmd, out);
    } else if (cmd.size() == 1 && cmd[0] == "keys") {
        return do_keys(cmd, out);
    } else if (cmd.size() == 4 && cmd[0] == "zadd") {
        return do_zadd(cmd, out);
    } else if (cmd.size() == 3 && cmd[0] == "zrem") {
        return do_zrem(cmd, out);
    } else if (cmd.size() == 3 && cmd[0] == "zscore") {
        return do_zscore(cmd, out);
    } else if (cmd.size() == 6 && cmd[0] == "zquery") {
        return do_zquery(cmd, out);
    } else {
        return out_err(out, ERR_UNKNOWN, "unknown command.");
    }
};