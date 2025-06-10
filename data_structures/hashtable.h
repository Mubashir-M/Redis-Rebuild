#ifndef HASHTABLE_H
#define HASHTABLE_H

#include <cstddef>
#include <cstdint>

struct HNode{
    HNode *next = NULL;
    uint64_t hcode = 0; //hash value
};

struct HTab{
    HNode **tab = NULL; // Array of slots
    size_t mask = 0; // power of 2 array size, 2^n - 1
    size_t size = 0; // number of keys
};

void h_init(HTab *htab, size_t n);
void h_insert(HTab *htab, HNode *node);
HNode **h_lookup(HTab *htab, HNode *key, bool (*eq)(HNode *, HNode *));
HNode *h_detach(HTab *htab, HNode **from);

#define container_of(ptr, type, member) \
    ((type *)((char *)(ptr) - offsetof(type, member)))
#endif