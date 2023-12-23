#include "trie.h"
#include "code.h"
#include <stdlib.h>

// Constructor for trie_node
TrieNode *trie_node_create(uint16_t index) {
    TrieNode *newNode = (TrieNode *) calloc(1, sizeof(TrieNode));
    newNode->code = index;

    for (int i = 0; i < ALPHABET; i++)
        newNode->children[i] = NULL;

    return newNode;
}

// Destructor for trie_node
void trie_node_delete(TrieNode *n) {
    n->code = 0; // set node code to 0
    free(n); // free memory from heap

    n = NULL;
}

// Initializes a trie_node
TrieNode *trie_create(void) {
    TrieNode *root = trie_node_create(EMPTY_CODE);

    if (root == NULL)
        return NULL;

    return root;
}

// Resets a trie_node to only contain the root node
void trie_reset(TrieNode *root) {
    for (int i = 0; i < ALPHABET; i++) {
        // if child exists
        if (root->children[i] != NULL) {
            trie_node_delete(root->children[i]); // delete that child node
            root->children[i] = NULL; // set this child to NULL
        }
    }
}

// deletes a sub-trie starting from node n
void trie_delete(TrieNode *n) {
    for (int i = 0; i < ALPHABET; i++) {
        if (n->children[i] != NULL) // if child exists
            trie_delete(
                n->children[i]); // delete that node and all its children by setting them to NULL
    }

    trie_node_delete(n);
}

// returns pointer of child containing symbol sym, or NULL if it doesn't exist
TrieNode *trie_step(TrieNode *n, uint8_t sym) {
    // iterate through all possible children
    for (int i = 0; i < ALPHABET; i++) {
        if (n->children[i] != NULL) // if child exists
            if (i == sym) // if child index is the same as sym
                return n->children[i];
    }

    return NULL; // no child with symbol sym found
}
