#ifndef ZSET_H
#define ZSET_H

#include "avltree.h"
#include "hashmap.h"

struct ZSet {
    AVLNode *root = NULL; // index by (scroe, name)
    HMap    hmap; // index by name
};

struct ZNode {
    // data structure nodes
    AVLNode tree;
    HNode hmap;

    // data
    double score = 0;
    size_t len = 0;
    char name[0];
};

struct HKey{
    HNode node;
    const char *name = NULL;
    size_t len = 0;
};

bool zset_insert(ZSet *zset, const char *name, size_t len, double score);
ZNode *zset_lookup(ZSet *zset, const char *name, size_t len);
void zset_delete(ZSet *zset, ZNode *node);
ZNode *zset_seekge(ZSet *zset, double score, const char *name, size_t len);
ZNode *znode_offset(ZNode *node, int64_t offset);
void zset_clear(ZSet *zset);
#endif