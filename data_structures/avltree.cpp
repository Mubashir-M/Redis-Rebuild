#include "avltree.h"

#include "assert.h"

static uint32_t max(uint32_t lhs, uint32_t rhs){
    return lhs < rhs ? rhs : lhs;
};

void avl_update(AVLNode *node){
    node->height = 1 + max(avl_height(node->left), avl_height(node->right));
    node->cnt = 1 + avl_cnt(node->left) + avl_cnt(node->right);
};
/*
static uint8_t avl_height_diff(AVLNode *node){
    uintptr_t p = (uintptr_t)node->parent;
    return p & 0b11; // Data from LSB
};
static AVLNode* avl_get_parent(AVLNode *node){
    uintptr_t p = (uintptr_t)node->parent; // clear the LSB
    return (AVLNode *)(p & (~0b11));
};
*/
static AVLNode *rot_left(AVLNode *node) {
    AVLNode *parent = node->parent;
    AVLNode *new_node = node->right;
    AVLNode *inner = new_node->left;
    // node <-> inner
    node->right = inner;
    if (inner) {
        inner->parent = node;
    }
    // parent <- new_node
    new_node->parent = parent;
    // new_node <-> node
    new_node->left = node;
    node->parent = new_node;
    // auxiliary data
    avl_update(node);
    avl_update(new_node);
    return new_node;
}

static AVLNode *rot_right(AVLNode *node){
    AVLNode *parent = node->parent;
    AVLNode *new_node = node->left;
    AVLNode *inner = new_node->right;

    // node <-> inner
    node->left = inner;
    if(inner){
        inner->parent = node;
    }
    // parent <- new_node
    new_node->parent = parent;
    // new_node <-> node
    new_node->right = node;
    node->parent = new_node;

    //auxiliary data
    avl_update(node);
    avl_update(new_node);

    return new_node;
};

static AVLNode* avl_fix_left(AVLNode *node){
    if(avl_height(node->left->left) < avl_height(node->left->right)){
        node->left = rot_left(node->left);
    }
    return rot_right(node);
};

static AVLNode* avl_fix_right(AVLNode *node){
    if(avl_height(node->right->right) < avl_height(node->right->left)){
        node->right = rot_right(node->right);
    }
    return rot_left(node);
};

AVLNode* avl_fix(AVLNode *node){

    while (true){
        AVLNode** from = &node;
        AVLNode *parent = node->parent;
        if(parent){
            from = parent->left == node ? &parent->left : &parent->right;
        }

        avl_update(node);

        uint32_t l = avl_height(node->left);
        uint32_t r = avl_height(node->right);

        if(l == r + 2){
            *from = avl_fix_left(node);
        } else if(l + 2 == r){
            *from  = avl_fix_right(node);
        }

        if(!parent){
            return *from;
        }
        node = parent;
    }
};

static AVLNode* avl_del_easy(AVLNode *node){
    assert(!node->left || !node->right); // at most 1 child

    AVLNode *child = node->left ? node->left : node->right;
    AVLNode *parent = node->parent;

    if(child){
        child->parent = parent;
    }

    if(!parent){
        return child;
    }

    AVLNode **from = parent->left == node ? &parent->left : &parent->right;
    *from = child;

    return avl_fix(parent);
};

AVLNode* avl_del(AVLNode *node){
    if(!node->left || !node->right){
        return avl_del_easy(node);
    }

    // find successor
    AVLNode* victim = node->right;
    while(victim->left){
        victim = victim->left;
    }

    // detach the successor
    AVLNode *root = avl_del_easy(victim);
    // swap with the successor
    *victim  = *node;
    if(victim->left){
        victim->left->parent = victim;
    }
    if(victim->right){
        victim->right->parent = victim;
    }

    // attach the successor to the parent, or update the root pointer
    AVLNode **from = &root;
    AVLNode *parent = node->parent;
    if(parent){
        from = parent->left == node ? &parent->left : &parent->right;
    }
    *from = victim;
    return root;
};

void search_and_insert(AVLNode **root, AVLNode *new_node, bool(*less)(AVLNode *, AVLNode *)){
    // find the insertion point
    AVLNode *parent = NULL;
    AVLNode **from = root;

    for(AVLNode *node = *root; node;){
        from = less(new_node, node) ? &node->left : &node->right;
        parent = node;
        node = *from;
    }

    // link the new node
    *from = new_node;
    new_node->parent = parent;

    // fix the updated root
    *root = avl_fix(new_node);
};

AVLNode* search_and_delete(AVLNode **root, uint32_t (*cmp)(AVLNode *, void *), void *key){
    for(AVLNode *node = *root; node;){
        int32_t r = cmp(node, key);
        if(r < 0){
            node = node->right;
        } else if (r > 0){
            node = node->left;
        } else {
            *root = avl_del(node);
            return node;
        }
    }
    return NULL;
};