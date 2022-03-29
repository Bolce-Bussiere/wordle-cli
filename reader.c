#include <stdio.h>
#include <stdlib.h>
#include "reader.h"

/*
 *  Calls handle(user_data, word) for each word of the desired length in file f
 *  If invalid is truthy for a character, the entire word that character is
 *  part of will be discarded. Characters that are neither valid nor invalid
 *  are treated as whitespace.
 */
void get_words(FILE *f,
               unsigned word_len,
               int (*valid)(int),
               int (*invalid)(int),
               void *user_data,
               void (*handle)(void *, char *))
{
    char *current_word = malloc((word_len + 1) * sizeof(char));
    char c;
    while (1)
    {
restart:
        for (unsigned i = 0; i < word_len; ++i)
        {
            c = fgetc(f);
            if (!valid(c))
            {
                ungetc(c, f);
                if (next_word(f, valid, invalid) == EOF)
                {
                    // break while loop
                    goto done;
                }
                goto restart;
            }
            current_word[i] = c;
        }
        c = fgetc(f);
        if (c == EOF)
        {
            goto done;
        } else if (valid(c) || invalid(c))
        {
            if (next_word(f, valid, invalid) == EOF)
            {
                goto done;
            }
        } else
        {
            current_word[word_len] = '\0';
            handle(user_data, current_word);
            ungetc(c, f);
            if (next_word(f, valid, invalid) == EOF)
            {
                goto done;
            }
        }
    }
done:
    free(current_word);
}

int next_word(FILE *f, int (*valid)(int), int (*invalid)(int))
{
    char c;
    do
    {
        c = fgetc(f);
    } while (c != EOF && (valid(c) || invalid(c)));
    if (c == EOF)
    {
        return EOF;
    }
    do
    {
        c = fgetc(f);
    } while (c != EOF && !valid(c) && !invalid(c));
    if (c == EOF)
    {
        return EOF;
    }
    return ungetc(c, f);
}
