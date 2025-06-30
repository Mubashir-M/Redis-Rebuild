#ifndef HEAP_H
#define HEAP_H

#include <cstdint>
#include <cstddef>
#include <vector>

struct HeapItem {
    uint64_t val; // heap value, the expiration time
    size_t *ref = NULL; // extra data associated with the value
};

void heap_update(HeapItem *a, size_t pos, size_t len);
void heap_delete(std::vector<HeapItem> &a, size_t pos);
void heap_upsert(std::vector<HeapItem> &a, size_t pos, HeapItem t);

#endif
