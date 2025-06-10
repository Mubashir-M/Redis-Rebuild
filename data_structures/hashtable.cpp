#include "hashtable.h"

#include <assert.h>

void h_init(HTab *htab, size_t n){
    assert((n > 0) && ((n - 1) & n) == 0); // n > 0 && n % 2 == 0
    htab->tab = (HNode **)calloc(n, sizeof(HNode *));
    htab->mask = n - 1;
    htab->size = 0;
};

void h_insert(HTab *htab, HNode *node){
    size_t pos = node->hcode & htab->mask; // hcode % (n-1)
    HNode *head = htab->tab[pos];
    node->next = head;
    htab->tab[pos] = node;
    htab->size++;
};

HNode **h_lookup(HTab *htab, HNode *key, bool (*eq)(HNode *, HNode *)){
    if(!htab->tab){
        return NULL;
    }
    size_t pos = key->hcode & htab ->mask;
    HNode **from = &htab->tab[pos];
    for(HNode *cur; (cur = *from) != NULL; from = &cur->next){
        if((cur->hcode == key->hcode) && eq(cur, key)){
            return from;
        }
    }
    return NULL;
};

HNode *h_detach(HTab *htab, HNode **from){
    HNode *node = *from;
    *from = node->next;
    htab->size--;
    return node;
};