#pragma once

int next_word(FILE *, int (*)(int), int (*)(int));
void get_words(FILE *, unsigned, int (*)(int), int (*)(int),
               void *, void (*)(void *, char *));
