#include "hashmap.h"
#include "hashtable.h"

#include <cstdlib>
#include <assert.h>


static void hm_trigger_rehashing(HMap *hmap){
    hmap->older = hmap->newer;
    h_init(&hmap->newer, (hmap->newer.mask + 1) * 2);
    hmap->migrate_pos = 0;
};

void hm_help_rehashing(HMap *hmap){
    size_t nwork = 0;
    while(nwork < k_rehashing_work && hmap->older.size > 0){
        // find a non-empty slot
        HNode **from = &hmap->older.tab[hmap->migrate_pos];
        if(!*from){
            hmap->migrate_pos++;
            continue;
        }
        // move the first list item to the newer table
        h_insert(&hmap->newer, h_detach(&hmap->older,from));
        nwork++;

        // discard the old table if done
        if(hmap->older.size == 0 && hmap->older.tab){
            free(hmap->older.tab);
            hmap->older = HTab{};
        }
    }
};

HNode *hm_lookup(HMap *hmap, HNode *key, bool (*eq)(HNode *, HNode *)){
    hm_help_rehashing(hmap);
    HNode **from = h_lookup(&hmap->newer, key, eq);
    if(!from){
        from = h_lookup(&hmap->older, key, eq);
    }
    return from ? *from : NULL;
};

void hm_insert(HMap *hmap, HNode *node){
    if(!hmap->newer.tab){
        h_init(&hmap->newer, 4);
    }
    h_insert(&hmap->newer, node);

    if(!hmap->older.tab){ // older is null --> not currently rehashing
        size_t threshold = (hmap->newer.mask + 1) * k_max_load_factor;
        if(hmap->newer.size >= threshold){
            hm_trigger_rehashing(hmap);
        }
    }
    hm_help_rehashing(hmap);
};

HNode *hm_delete(HMap *hmap, HNode *key, bool (*eq)(HNode *, HNode *)){
    hm_help_rehashing(hmap);
    if(HNode **from = h_lookup(&hmap->newer, key, eq)){
        return h_detach(&hmap->newer, from);
    }

    if(HNode **from = h_lookup(&hmap->older, key, eq)){
        return h_detach(&hmap->older, from);
    }

    return NULL;
};

void hm_clear(HMap *hmap){
    free(hmap->older.tab);
    free(hmap->newer.tab);
    *hmap = HMap();
};

size_t hm_size(HMap *hmap){
    return hmap->newer.size + hmap->older.size;
};

void hm_foreach(HMap *hmap, bool (*f)(HNode *, void *), void *arg){
    h_foreach(&hmap->newer, f, arg) && h_foreach(&hmap->older, f, arg);
};