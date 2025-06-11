#include "data_store.h"
#include "server_config.h"
#include "buffer_operations.h"
#include "assert.h"

HMap g_data;

bool entry_eq(HNode *lhs, HNode *rhs){
    struct Entry *le = container_of(lhs, struct Entry, node);
    struct Entry *re = container_of(rhs, struct Entry, node);
    return le->key == re->key;
};

static uint64_t str_hash(const uint8_t *data, size_t len){
    uint32_t h = 0x811C9DC5;
    for (size_t i = 0; i< len; i++){
        h = (h + data[i]) * 0x01000193;
    }
    return h;
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

void out_err(Buffer &out, uint32_t code, const std::string &msg){
    buf_append_u8(out, TAG_ERR);
    buf_append_u32(out, code);
    buf_append_u32(out, (uint32_t)msg.size());
    buf_append(out, (const uint8_t*)msg.data(), msg.size());
};

static void do_get(std::vector<std::string> &cmd, Buffer &out){
    // a dummy `Entry` just for the lookup
    Entry key;
    key.key.swap(cmd[1]);
    key.node.hcode = str_hash((uint8_t *)key.key.data(), key.key.size());
    // hashtable lookup
    HNode *node = hm_lookup(&g_data, &key.node, &entry_eq);
    if (!node) {
        return out_nil(out);
    }
   
    const std::string &val = container_of(node, Entry, node)->val;
    return out_str(out, val.data(), val.size());
};

static void do_set(std::vector<std::string>& cmd, Buffer &out){
    Entry key;
    key.key.swap(cmd[1]);
    key.node.hcode = str_hash((uint8_t *)key.key.data(), key.key.size());

    HNode *node = hm_lookup(&g_data, &key.node, &entry_eq);

    if(node){
        container_of(node, Entry, node)->val.swap(cmd[2]);
    } else {
        Entry *ent = new Entry();
        ent->key.swap(key.key);
        ent->node.hcode= key.node.hcode;
        ent->val.swap(cmd[2]);
        hm_insert(&g_data, &ent->node);
    }
    return out_nil(out);
};

static void do_del(std::vector<std::string> &cmd, Buffer &out){
    Entry key;
    key.key.swap(cmd[1]);

    key.node.hcode = str_hash((uint8_t *)key.key.data(), key.key.size());
    HNode *node = hm_delete(&g_data, &key.node, &entry_eq);
    if(node){
        delete container_of(node, Entry, node);
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
    out_arr(out, (uint32_t)hm_size(&g_data));
    hm_foreach(&g_data, &cb_keys, (void *)&out);
};



void do_request(std::vector<std::string> &cmd, Buffer &out){
    if (cmd.size() == 2 && cmd[0] == "get"){
        return do_get(cmd, out);
    } else if (cmd.size() == 3 && cmd[0] == "set"){
        return do_set(cmd, out);
    } else if (cmd.size() == 2 && cmd[0] == "del"){
        return do_del(cmd, out);
    } else if (cmd.size() == 1 && cmd[0] == "keys") {
        return do_keys(cmd, out);
    } else 
    return out_err(out, ERR_UNKNOWN, "unknown command.");
};