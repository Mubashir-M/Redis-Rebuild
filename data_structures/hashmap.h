#ifndef HASHMAP_H
#define HASHMAP_H

#include "hashtable.h"

struct HMap
{
    HTab newer;
    HTab older;
    size_t migrate_pos = 0;
};

const size_t k_max_load_factor = 8;
const size_t k_rehashing_work = 128;

HNode *hm_lookup(HMap *hmap, HNode *key, bool (*eq)(HNode *, HNode *));
void hm_insert(HMap *hmap, HNode *node);
HNode *hm_delete(HMap *hmap, HNode *key, bool (*eq)(HNode *, HNode *));
void hm_clear(HMap *hmap);
size_t hm_size(HMap *hmap);
void hm_foreach(HMap *hmap, bool (*f)(HNode *, void *), void *arg);
#endif