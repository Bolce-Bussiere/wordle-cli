#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include "hashmap.h"
#include "reader.h"

struct color
{
    unsigned char fg, bg;
};

const char *KEYBOARD[] =
{
    "qwertyuiop",
     "asdfghjkl",
      "zxcvbnm"
};

void set_color(struct color c) {
    printf("\x1b[38;5;%um\x1b[48;5;%um", c.fg, c.bg);
}

void print_keys(struct color key_colors[26])
{
    printf("\x1b[3;0H"); // cursor to line 3
    for (int i = 0; i < 3; ++i)
    {
        printf("      ");
        for (int j = 0; j < i; ++j)
        {
            printf(" ");
        }
        for (int j = 0; KEYBOARD[i][j]; ++j)
        {
            set_color(key_colors[KEYBOARD[i][j] - 'a']);
            printf("%c", KEYBOARD[i][j]);
        }
        printf("\x1b[39;49m\n");
    }
}

void get_colors(struct color *colors, const char *s1, const char *s2, unsigned len)
{
    static unsigned set[256] = {0};
    for (unsigned i = 0; i < len; ++i)
    {
        if (s2[i] == s1[i])
        {
            colors[i].fg = 0;
            colors[i].bg = 82;
        }
        else
        {
            ++set[(unsigned char) s2[i]];
        }
    }
    for (unsigned i = 0; i < len; ++i)
    {
        if (s2[i] != s1[i])
        {
            if (set[(unsigned char) s1[i]] > 0)
            {
                colors[i].fg = 0;
                colors[i].bg = 220;
                --set[(unsigned char) s1[i]];
            }
            else
            {
                colors[i].fg = 255;
                colors[i].bg = 0;
            }
        }
    }
    for (unsigned i = 0; i < len; ++i)
    {
        set[(unsigned char) s2[i]] = 0;
    }
}

// Command-line options
struct options
{
    FILE *word_file;
    unsigned word_length;
    int hard_mode;
};

void add_word(struct map *words, char *word)
{
    for (int i = 0; word[i]; ++i)
    {
        word[i] = tolower(word[i]);
    }
    ++*map_value_ptr(words, word, 0);
}

// don't include words with apostrophes or numbers
int invalid(int c)
{
    return c == '\'' || (c >= '0' && c <= '9');
}

int wordle(struct options opts)
{
    struct color key_colors[26];
    for (int i = 0; i < 26; ++i)
    {
        key_colors[i].fg = 0;
        key_colors[i].bg = 255;
    }
    struct map *words = map_new(10000);
    if (!words)
    {
        return 1;
    }
    get_words(opts.word_file, opts.word_length, isalpha, invalid, words, add_word);
    if (words->elems == 0)
    {
        printf("No words in file matched criteria!\n");
        return 1;
    }
    char *secret = map_random_key(words);
    
    // Make the guess buffer a bit longer than neccessary to avoid reallocs
    size_t guess_buff_size = 2 * (opts.word_length + 1);
    char *guess = malloc(guess_buff_size * sizeof(char));
    ssize_t guess_len;
    struct color *colors = malloc(opts.word_length * sizeof(struct color));
    
    int done = 0, guesses = 0;
    printf("\x1b[H\x1b[2J"); // Move cursor to 0, 0 and clear screen
    printf("\x1b[1m\tWORDLE\x1b[22m\n"); // bold
    print_keys(key_colors);
    printf("Guess a %d letter word:\n", opts.word_length);
    while (!done)
    {
        guess_len = getline(&guess, &guess_buff_size, stdin);
        if (guess_len < 0)
        {
            fprintf(stderr, "AAAAAAHHH HOLY FUCK ENOMEM\n");
            return 2;
        }
        guess[--guess_len] = '\0'; // remove newline
        if (map_has(words, guess) && guess_len == opts.word_length)
        {
            done = 1;
            printf("\x1b[1F"); // Cursor to begining of previous line
            printf("\x1b[0J"); // Clear from cursor to end of screen (clear errors)
            get_colors(colors, guess, secret, opts.word_length);
            for (unsigned i = 0; i < opts.word_length; ++i)
            {
                set_color(colors[i]);
                printf("%c", guess[i]);
                if (colors[i].bg != 82)
                {
                    done = 0;
                }
                if (key_colors[guess[i] - 'a'].bg != 82)
                {
                    key_colors[guess[i] - 'a'] = colors[i];
                }
            }
            ++guesses;
            printf("\x1b[39;49m"); // reset text color
            printf("\x1b[s"); // save cursor position
            print_keys(key_colors);
            printf("\x1b[u"); // restore cursor position
            printf("\n");
        }
        else
        {
            // Clear line
            printf("\x1b[2K\r");
            if (guess_len < opts.word_length)
            {
                printf("Guess too short!");
            }
            else if (guess_len > opts.word_length)
            {
                printf("Guess too long!");
            }
            else
            {
                printf("Not a valid word!");
            }
            // Reset cursor to before input
            printf("\x1b[1A");
            // Clear line
            printf("\x1b[2K\r");
        }
    }
    printf("Nice job! You got it in %d guesses!\n", guesses);
    map_free(words);
    return 0;
}

int main(int argc, char **argv)
{
    struct options opts = {
        .word_file = NULL,
        .word_length = 5,
        .hard_mode = 0
    };
    for (int i = 1; i < argc; ++i)
    {
        if (strcmp(argv[i], "--length") == 0)
        {
            ++i;
            if (i >= argc)
            {
                printf("Error: no argument for option '--length',"
                       " expected an integer\n");
            }
            char *endptr;
            opts.word_length = strtol(argv[i], &endptr, 10);
            if (*endptr != '\0')
            {
                printf("Error: option '--length' expected an integer argument,"
                       " instead got '%s'\n", argv[i]);
                return 1;
            }
        }
        else if (strcmp(argv[i], "--file") == 0)
        {
            ++i;
            if (i >= argc)
            {
                printf("Error: no argument for option '--file',"
                       " expected a file name\n");
            }
            opts.word_file = fopen(argv[i], "r");
            if (!opts.word_file)
            {
                printf("Error: could not open file for reading: %s\n", argv[i]);
                return 1;
            }
        }
//        else if (strcmp(argv[i], "--hard") == 0)
//        {
//            opts.hard_mode = 1;
//        }
    }
    if (opts.word_file == NULL)
    {
        // Try to open words.txt
        opts.word_file = fopen("words.txt", "r");
        if (opts.word_file == NULL)
        {
            printf("No word file supplied and default 'words.txt' failed to open.\n");
            return 1;
        }
    }
    srand(time(NULL));
    wordle(opts);
    return 0;
}
