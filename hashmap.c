#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include "hashmap.h"

const double salt[16] = {
    3.141592653589793238, 4.626433832795028841,
    9.716939937510582097, 4.944592307816406286,

    0.577215664901532860, 6.065120900824024310,
    4.159335939923598805, 7.672348848677267776,

    1.202056903159594285, 3.997381615114499907,
    6.498629234049888179, 2.271555341838205786,

    1.036927755143369926, 3.313654864570341680,
    5.708091950191281197, 4.192677903803589786
};

const double PHI = 1.61803398874989484820458683436563811772;

size_t hash(const char *key, size_t arr_len) {
    long double total = 0;
    unsigned i = 0;
    while (key[i] != '\0') {
        total += salt[i & 0x0f] * key[i];
        ++i;
    }
    return arr_len * (PHI * total - floor(PHI * total));
}

struct map *map_new(size_t array_size) {
    struct map *new_map = malloc(sizeof(struct map));
    if (new_map == NULL) {
        return NULL;
    }
    new_map->elems = 0;
    new_map->collisions = 0;
    new_map->arr_len = array_size;
    new_map->arr = calloc(array_size, sizeof(struct entry *));
    if (new_map->arr == NULL) {
        free(new_map);
        return NULL;
    }
    return new_map;
}

void map_insert(struct map *m, const char *key, int value) {
    struct entry **current = &(m->arr[hash(key, m->arr_len)]);
    while (*current != NULL) {
        if (strcmp((*current)->key, key) == 0) {
            (*current)->value = value;
            return;
        }
        current = &((*current)->next);
    }
    if (current != &(m->arr[hash(key, m->arr_len)])) {
        ++(m->collisions);
    }
    // make a copy of key
    char *new_key = malloc((strlen(key) + 1) * sizeof(char));
    strcpy(new_key, key);

    *current = malloc(sizeof(struct entry));
    (*current)->key = new_key;
    (*current)->value = value;
    (*current)->next = NULL;
    ++(m->elems);
}

void map_remove(struct map *m, const char *key) {
    struct entry **current = &(m->arr[hash(key, m->arr_len)]);
    while (*current != NULL) {
        if (strcmp((*current)->key, key) == 0) {
            struct entry *temp = *current;
            *current = temp->next;
            free(temp->key);
            free(temp);
            --(m->elems);
            return;
        }
        current = &((*current)->next);
    }
}

int *map_value_ptr(struct map *m, const char *key, int default_val) {
    struct entry **current = &(m->arr[hash(key, m->arr_len)]);
    while (*current != NULL) {
        if (strcmp((*current)->key, key) == 0) {
            return &((*current)->value);
        }
        current = &((*current)->next);
    }
    if (current != &(m->arr[hash(key, m->arr_len)])) {
        ++(m->collisions);
    }
    // make a copy of key
    char *new_key = malloc((strlen(key) + 1) * sizeof(char));
    strcpy(new_key, key);

    *current = malloc(sizeof(struct entry));
    (*current)->key = new_key;
    (*current)->value = default_val;
    (*current)->next = NULL;
    ++(m->elems);
    return &((*current)->value);
}

int map_has(struct map *m, const char *key) {
    struct entry **current = &(m->arr[hash(key, m->arr_len)]);
    while (*current != NULL) {
        if (strcmp((*current)->key, key) == 0) {
            return 1;
        }
        current = &((*current)->next);
    }
    return 0;
}

void map_free(struct map *m) {
    struct entry *curr, *temp;
    for (unsigned i = 0; i < m->arr_len; ++i) {
        curr = m->arr[i];
        while (curr != NULL) {
            temp = curr->next;
            free(curr->key);
            free(curr);
            curr = temp;
        }
    }
    free(m->arr);
    free(m);
}

char *map_random_key(struct map *m) {
    unsigned choice_ind = rand() % m->elems,
             entry_ind = 0;
    struct entry *e = NULL;
    for (unsigned i = 0; i < m->arr_len; ++i) {
        for (e = m->arr[i]; e != NULL; e = e->next) {
            ++entry_ind;
            if (entry_ind == choice_ind) {
                return e->key;
            }
        }
    }
    assert(0);
    return NULL;
}
