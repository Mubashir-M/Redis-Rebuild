#include "data_store.h"
#include "server_config.h"

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

static void do_get(std::vector<std::string> &cmd, Response &out){
    // a dummy `Entry` just for the lookup
    Entry key;
    key.key.swap(cmd[1]);
    key.node.hcode = str_hash((uint8_t *)key.key.data(), key.key.size());
    // hashtable lookup
    HNode *node = hm_lookup(&g_data, &key.node, &entry_eq);
    if (!node) {
        out.status = RES_NX;
        return;
    }
    // key found!
    out.status = RES_OK;

    const Entry *stored_entry = container_of(node, Entry, node);
    out.value_ptr = &stored_entry->val; 
};

static void do_set(std::vector<std::string>& cmd, Response &){
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
};

static void do_del(std::vector<std::string> &cmd, Response &){
    Entry key;
    key.key.swap(cmd[1]);

    key.node.hcode = str_hash((uint8_t *)key.key.data(), key.key.size());
    HNode *node = hm_delete(&g_data, &key.node, &entry_eq);
    if(node){
        delete container_of(node, Entry, node);
    }
};

void do_request(std::vector<std::string> &cmd, Response &out){
    if (cmd.size() == 2 && cmd[0] == "get"){
        return do_get(cmd, out);
        //out.value_ptr = &it->second;
    } else if (cmd.size() == 3 && cmd[0] == "set"){
        return do_set(cmd, out);
    } else if (cmd.size() == 2 && cmd[0] == "del"){
        return do_del(cmd, out);
    } else {
        out.status = RES_ERR;
    }
};