#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <signal.h>
#include "hashmap.h"
#include "reader.h"

const char *USAGE =
    "Usage: %s [OPTIONS]\n"
    "OPTIONS:\n"
    "   --file {filename} : Use filename as dictionary, default is words.txt\n"
    "   --length {n}      : Use words of length n\n"
    "   --guesses {n}     : Maximum number of allowed guesses. 0 for unlimited\n";

struct palette
{
    unsigned char fg, bg;
};

enum color {
    WHITE = 0,
    BLACK,
    YELLOW,
    GREEN
};

struct palette THEME[] = {
    [WHITE]  = { .fg =   0, .bg = 255 },
    [YELLOW] = { .fg =   0, .bg = 220 },
    [GREEN]  = { .fg =   0, .bg =  82 },
    [BLACK]  = { .fg = 255, .bg =   0 }
};

const char *KEYBOARD[] =
{
    "qwertyuiop",
     "asdfghjkl",
      "zxcvbnm"
};

void set_color(enum color c) {
    printf("\x1b[38;5;%um\x1b[48;5;%um", THEME[c].fg, THEME[c].bg);
}

void print_keys(enum color key_colors[26], unsigned line)
{
    printf("\x1b[%u;0H", line); // cursor to line
    for (int i = 0; i < 3; ++i)
    {
        printf("  ");
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

void get_colors(enum color *colors, const char *s1, const char *s2, unsigned len)
{
    static unsigned set[256];
    for (unsigned i = 0; i < len; ++i)
    {
        set[(unsigned char) s2[i]] = 0;
        set[(unsigned char) s1[i]] = 0;
    }
    for (unsigned i = 0; i < len; ++i)
    {
        if (s2[i] == s1[i])
        {
            colors[i] = GREEN;
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
                colors[i] = YELLOW;
                --set[(unsigned char) s1[i]];
            }
            else
            {
                colors[i] = BLACK;
            }
        }
    }
}

// for reading dict file
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

// Command-line options
struct options
{
    FILE *word_file;
    unsigned word_length;
    unsigned guesses;
    int hard_mode;
};

// scary global variable so we can print secret word in signal handler
char *secret_GLOBAL;

// Handle Ctrl-C : Print the secret word and exit
void kb_interrupt_handler(int signum)
{
    // I don't want to add more global state, so assume we recived
    // Ctrl-C while waiting for user input
    printf("\x1b[1E"); // Go down 1 line
    printf("\x1b[0J"); // Clear to end of screen
    printf("The secret word was %s.\n", secret_GLOBAL);
    exit(signum);
}

int wordle(struct options opts)
{
    enum color key_colors[26];
    for (int i = 0; i < 26; ++i)
    {
        key_colors[i] = WHITE;
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
    secret_GLOBAL = secret;
    // Now that we have a secret word set it is safe to set our interrupt handler
    signal(SIGINT, kb_interrupt_handler);
    
    // Make the guess buffer a bit longer than neccessary to avoid reallocs
    size_t guess_buff_size = 2 * (opts.word_length + 1);
    char *guess = malloc(guess_buff_size * sizeof(char));
    ssize_t guess_len;
    enum color *colors = malloc(opts.word_length * sizeof(enum color));
    
    int correct = 0, guesses = 0;
    printf("\x1b[H\x1b[2J"); // Move cursor to 0, 0 and clear screen
    printf("\x1b[1m    WORDLE\x1b[22m\n"); // bold
    print_keys(key_colors, 3);
    for (int i = 0; i < opts.guesses; ++i) {
        printf("\n");
        for (int j = 0; j < opts.word_length; ++j) {
            printf("_"); // haha emoji face
        }
    }
    printf("\x1b[7;0H");
    while (!correct && (opts.guesses == 0 || guesses < opts.guesses))
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
            ++guesses;
            correct = 1;
            printf("\x1b[1F"); // Cursor to begining of previous line
            if (opts.guesses != 0)
            {
                printf("\x1b[%u;0H", opts.guesses + 7);
                printf("\x1b[2K");
                printf("\x1b[%u;0H", guesses + 6);
            }
            else
            {
                printf("\x1b[0J");
            }
            get_colors(colors, guess, secret, opts.word_length);
            for (unsigned i = 0; i < opts.word_length; ++i)
            {
                set_color(colors[i]);
                printf("%c", guess[i]);
                if (colors[i] != GREEN)
                {
                    correct = 0;
                }
                if (key_colors[guess[i] - 'a'] < colors[i])
                {
                    key_colors[guess[i] - 'a'] = colors[i];
                }
            }
            printf("\x1b[39;49m"); // reset text color
            printf("\x1b[s"); // save cursor position
            print_keys(key_colors, 3); // TODO: change this towards 0 as
                                       // no. guesses approches lines in term
            printf("\x1b[u"); // restore cursor position
            printf("\n");
        }
        else
        {
            if (opts.guesses != 0)
            {
                printf("\x1b[%u;0H", opts.guesses + 7);
            }
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
            printf("\x1b[%u;0H", guesses + 7);
            // Clear line
            printf("\x1b[2K\r");
            // Print spaces
            for (int i = 0; i < opts.word_length; ++i) {
                printf("_");
            }
            printf("\r");
            fflush(stdout);
        }
    }
    if (correct)
    {
        printf("Nice job! You got it in %d guesses!\n", guesses);
    }
    else
    {
        printf("So close! The word was %s.\n", secret);
    }
    printf("\x1b[0J");
    map_free(words);
    return 0;
}

int main(int argc, char **argv)
{
    struct options opts = {
        .word_file = NULL,
        .word_length = 5,
        .guesses = 6,
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
        else if (strcmp(argv[i], "--guesses") == 0) {
            ++i;
            if (i >= argc) {
                printf("Error: option '--guesses' needs an argument");
                return 1;
            }
            char *endptr;
            opts.guesses = strtol(argv[i], &endptr, 10);
            if (*endptr != '\0')
            {
                printf("Error: option '--guesses' expected an integer argument,"
                       " instead got '%s'\n", argv[i]);
                return 1;
            }
        }
        else if (strcmp(argv[i], "--help") == 0)
        {
            printf(USAGE, argv[0]);
            return 0;
        }
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
