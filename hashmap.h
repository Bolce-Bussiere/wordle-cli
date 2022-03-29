#pragma once

struct entry {
    char *key;
    int value;
    struct entry *next;
};

struct map {
    size_t elems;
    size_t collisions;
    size_t arr_len;
    struct entry **arr;
};

size_t hash(const char *, size_t);
struct map *map_new(size_t);
void map_insert(struct map *, const char *, int);
void map_remove(struct map *, const char *);
int *map_value_ptr(struct map *, const char *, int);
int map_has(struct map *, const char *);
void map_free(struct map *);
char *map_random_key(struct map *);
