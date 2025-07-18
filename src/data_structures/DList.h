#ifndef DLIST_H
#define DLIST_H

struct DList {
    DList *prev = nullptr;
    DList *next = nullptr;
};

inline void dlist_detach(DList *node){
    DList *prev = node->prev;
    DList *next = node->next;
    prev->next = next;
    next->prev = prev;
};

inline void dlist_init(DList *node){
    // Dummy node linked to itself
    node->prev = node->next = node;
};

inline bool dlist_empty(DList *node){
    return node->next == node;
};

inline void dlist_insert_before(DList *target, DList *rookie){
    DList *prev = target->prev;
    prev->next = rookie;
    rookie->prev = prev;
    rookie->next = target;
    target->prev = rookie;
};


#endif