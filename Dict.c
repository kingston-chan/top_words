// COMP2521 21T2 Assignment 1
// Dict.c ... implementation of the Dictionary ADT

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "Dict.h"
#include "WFreq.h"

#define MAX_SIZE 26

typedef struct tree_node *T_Node;

// Tree node structs that hold the left and right 
// children, the word, and the frequencies that occur
// in the given file
struct tree_node {
    T_Node l;       // left child
    T_Node r;       // right child
    char *word;     // given word
    int count;      // frequency count
};

typedef struct trees *Trees;

// Tree struct that holds the root of the tree,
// the size of the tree and which letter of the 
// alphabet it contains
struct trees {
    T_Node root;    // root to tree that holds words with
                    // its first letter corresponding to 
                    // the trees letter
    int ch;         // the letter that determines words
                    // to be stored in the tree
};

// Dictionary struct that holds an array of tree structs,
// the number of trees created and the total number of unique
// words added into the dictionary
struct DictRep {
    Trees *tree_array;  // array of trees structs
    int num_tree;       // number of trees in the array
    int num_words;      // total number of unique words that 
                        // is stored in the dictionary
};

// DictNew/Creating new nodes/trees helper functions
static T_Node new_node(char *word);
static Trees new_tree(void);

// DictFree helper function
static void free_tree_nodes(T_Node tree);

// DictInsert helper function
static T_Node insert_tree(T_Node n, char *word, Dict d, int *exists);

// AVL tree helper functions
static T_Node rotate_right(T_Node t);
static T_Node rotate_left(T_Node t);
static int tree_height(T_Node t);

// DictFind helper function
static int tree_find(T_Node t, char *word);

// DictFindTopN helper functions
static void dict_array(T_Node t, WFreq *w_array, int *i);
static int cmpfunc(const void * a, const void * b);

// Tree struct array helper functions
static int char_exists(Dict d, int ch);
static void insert_new_tree(Dict d, int ch);
static int find_pos_letter(char *word);


// Creates a new Dictionary
Dict DictNew(void) {
    Dict d = malloc(sizeof(*d));
    if (d == NULL) {
        fprintf(stderr, "Memory allocation failed for Dict\n");
        return NULL;
    }
    d->num_words = 0;
    d->num_tree = 0;
    // Malloc an array of trees
    d->tree_array = malloc(sizeof(Trees)*MAX_SIZE);
    if (d->tree_array == NULL) {
        fprintf(stderr, "Memory allocation failed for tree_array\n");
        return NULL;
    }
    // Each tree in the array is set to NULL
    for (int i = 0; i < MAX_SIZE; i++) {
        d->tree_array[i] = NULL;
    }
    return d;
}

// Frees the given Dictionary
void DictFree(Dict d) {
    // Frees the memory of each tree in the array
    for (int i = 0; i < MAX_SIZE; i++) {
        // If there are non-empty trees need to free the nodes aswell
        if (i < d->num_tree) free_tree_nodes(d->tree_array[i]->root);
        free(d->tree_array[i]);
    }
    free(d->tree_array);
    free(d);
}

// Inserts an occurrence of the given word into the Dictionary
void DictInsert(Dict d, char *word) {
    int exists = 0;
    int pos;
    // Check if there are any non-empty trees
    if (d->num_tree == 0) {
        pos = 0;
        // Find position of the first character of the word
        int f_ch = find_pos_letter(word);
        insert_new_tree(d, f_ch);
        d->num_tree++;
    } else {
        // There exists a non-empty tree
        int ch = find_pos_letter(word);
        // Find if the word's first character is already has a tree
        int k = char_exists(d, ch);
        if (k >= 0) {
            // Since char_exists returns the position in which the 
            // character is stored in the array, set it to the pos
            pos = k;
        } else {
            // First character does not exist so add new tree to 
            // array
            insert_new_tree(d, ch);
            // Position in the array becomes the current size of the
            // array
            pos = d->num_tree;
            d->num_tree++;
        }
    }
    d->tree_array[pos]->root = insert_tree(d->tree_array[pos]->root, word, d, &exists);
}

// Returns the occurrence count of the given word. Returns 0 if the word
// is not in the Dictionary.
int DictFind(Dict d, char *word) {
    // Check if there are trees
    if (d->num_tree == 0) return 0;
    int ch = find_pos_letter(word);
    int i = char_exists(d, ch);
    // If char exists in the tree array, it is possible it is in that tree
    if (i >= 0) return tree_find(d->tree_array[i]->root, word);
    return 0;
}

// Finds  the top `n` frequently occurring words in the given Dictionary
// and stores them in the given  `wfs`  array  in  decreasing  order  of
// frequency,  and then in increasing lexicographic order for words with
// the same frequency. Returns the number of WFreq's stored in the given
// array (this will be min(`n`, #words in the Dictionary)) in  case  the
// Dictionary  does  not  contain enough words to fill the entire array.
// Assumes that the `wfs` array has size `n`.
int DictFindTopN(Dict d, WFreq *wfs, int n) {
    // Malloc a WFreq struct array of total unique words in the dictionary size
    WFreq *w_arr = malloc(d->num_words*sizeof(*w_arr));
    if (w_arr == NULL) {
        fprintf(stderr, "Memory allocation failed for wfs array sort\n");
        return -1;
    }
    int i = 0;
    // Dump all values into the struct array from all the trees
    for (int k = 0; k < d->num_tree; k++) {
        dict_array(d->tree_array[k]->root, w_arr, &i);
    }
    // Sort the array
    qsort(w_arr, d->num_words, sizeof(WFreq), cmpfunc);
    // Limit the size of n to the total unique words in the dictionary
    if (n > d->num_words) n = d->num_words;
    // Copy the memory from the array of all sorted words by frequency from 
    // the dictionary into the given WFreq struct array
    memcpy(wfs, w_arr, sizeof(*w_arr) * n);
    free(w_arr);
    return n;
}

// Displays the given Dictionary. This is purely for debugging purposes,
// so  you  may  display the Dictionary in any format you want.  You may
// choose not to implement this.
void DictShow(Dict d) {
    
}

/*****Start of helper functions*****/

/*****DictNew/Creating new nodes/trees helper functions*****/

// Create a new tree 
static Trees new_tree(void) {
    Trees n_tree = malloc(sizeof(*n_tree));
    if (n_tree == NULL) {
        fprintf(stderr, "Memory allocation failed for new tree\n");
        return NULL;
    }
    n_tree->ch = 0;
    n_tree->root = NULL;
    return n_tree;
}

// Creates a new tree node
static T_Node new_node(char *word) {
    T_Node n = malloc(sizeof(*n));
    if (n == NULL) {
        fprintf(stderr, "Memory allocation failed for new tree node\n");
        return NULL;
    }
    n->l = NULL;
    n->r = NULL;
    n->word = word;
    n->count = 1;
    return n;
}

/*****DictFree helper function*****/

// Free the tree nodes recursively
static void free_tree_nodes(T_Node tree) {
    if (tree == NULL) return;
    free_tree_nodes(tree->l);
    free_tree_nodes(tree->r);
    // Need to free the word since strdup was used
    free(tree->word);
    free(tree);
}


/*****DictInsert helper function*****/

// Adds the given word to the AVL tree
static T_Node insert_tree(T_Node n, char *word, Dict d, int *exists) {
    if (n == NULL) {
        // Empty tree
        d->num_words++;
        return new_node(strdup(word));
    } else if (strcmp(n->word, word) < 0) {
        // Given word appears after current word in
        // lexicographical order
        n->r = insert_tree(n->r, word, d, exists);
    } else if (strcmp(n->word, word) > 0) {
        // Given word appears before current word in
        // lexicographical order
        n->l = insert_tree(n->l, word, d, exists);
    } else {
        // Found an occurance of given word in dictionary
        n->count += 1;
        *exists = 1;
    }
    // Check if inserted word already exists in dictonary
    // If it does exist, no need to balance the tree
    if(!*exists) {
        // Height of left and right sub tree
        int left_height = tree_height(n->l);
        int right_height = tree_height(n->r);
        
        // Check if the node is balanced or not
        if ((left_height - right_height) > 1) {
            if (strcmp(n->l->word, word) < 0) {
                n->l = rotate_left(n->l);
            }
            n = rotate_right(n);
        } else if ((right_height - left_height) > 1) {
            if (strcmp(n->r->word, word) > 0) {
                n->r = rotate_right(n->r);
            }
            n = rotate_left(n);
        }
    }
    return n;
}

/*****AVL tree helper functions*****/

// Rotate node of tree right
static T_Node rotate_right(T_Node t) {
    T_Node new_root = t->l;
    if (t == NULL || new_root == NULL) return t;
    t->l = new_root->r;
    new_root->r = t;
    return new_root;
}

// Rotate node of tree left
static T_Node rotate_left(T_Node t) {
    T_Node new_root = t->r;
    if (t == NULL || new_root == NULL) return t;
    t->r = new_root->l;
    new_root->l = t;
    return new_root;
}

// Returns the height of the tree
static int tree_height(T_Node t) {
    if (t == NULL) {
        return -1;
    } else {
        int l = tree_height(t->l);
        int r = tree_height(t->r);
        return 1 + ((l > r) ? l : r);
    }
}

/*****DictFind helper function*****/

// Returns 1 if word is in the tree, else return 0
static int tree_find(T_Node t, char *word) {
    if (t == NULL) {
        // Cannot find word
        return 0;
    } else if (strcmp(t->word, word) < 0) {
        // Given word is greater than in lexicographical order
        return tree_find(t->r, word);
    } else if (strcmp(t->word, word) > 0) {
        // Given word is less than in lexicographical order
        return tree_find(t->l, word);
    } else {
        // Found word
        return 1;
    }
}

/*****DictFindTopN helper functions*****/

// Dump all the values from the tree into the array recursively
static void dict_array(T_Node t, WFreq *w_arr, int *i) {
    if (t == NULL) return;
    w_arr[*i].word = t->word;
    w_arr[*i].freq = t->count;
    ++*i;
    dict_array(t->l, w_arr, i);
    dict_array(t->r, w_arr, i);
}


// Function that allows qsort to sort the frequencies in
// descending order
static int cmpfunc(const void *a, const void *b) {
    WFreq *i1 = (WFreq *)a;
    WFreq *i2 = (WFreq *)b;
    
    if (i1->freq < i2->freq) {
        return 1;
    } else if (i1->freq > i2->freq) {
        return -1;
    } else {
        // If both values are the same return the 
        // sort by lexicographical order
        return (strcmp(i1->word, i2->word));
    }

}

/*****Tree struct array helper functions*****/

// Find existing tree with the character and return its position in array, 
// if does not exist return -1;
static int char_exists(Dict d, int ch) {
    for (int i = 0; i < d->num_tree; i++) {
        if (ch == d->tree_array[i]->ch) return i;
    }
    return -1;
}

// Insert a new tree into the tree array
static void insert_new_tree(Dict d, int ch) {
    d->tree_array[d->num_tree] = new_tree();
    if (ch > 0) {
        d->tree_array[d->num_tree]->ch = ch;
    }
}

// Finds the first character of the word's position
// on the alphabet, since ' and - are valid
// they can be the same position as 'a'
static int find_pos_letter(char *word) {
    if (word[0] > 'a') return word[0] - 'a';
    return 0;
}



