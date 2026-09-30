#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_HEADERS 32
typedef struct
{
    char *keys[MAX_HEADERS];
    char *vals[MAX_HEADERS];
    int count;
} Headers;

static char *skip_ws(char *s)
{
    while (*s == ' ' || *s == '\t')
        s++;
    return s;
}

static void parse_headers(char *text, Headers *h)
{
    for (char *line = strtok(text, "\n"); line != NULL; line = strtok(NULL, "\n"))
    {
        char *colon = strchr(line, ':');

        if (colon == NULL)
            continue;

        *colon = '\0';
        char *key = line;
        char *val = skip_ws(colon + 1);

        if (h->count < MAX_HEADERS)
        {
            h->keys[h->count] = key;
            h->vals[h->count] = val;
            h->count++;
        }
    }
}

int main(void)
{

    char raw[] =
        "Host: example.com\n"
        "Accept: */*\n"
        "Connection\n"
        "User-Agent: memdbg-cli\n";

    Headers h = {.count = 0};
    parse_headers(raw, &h);

    printf("parsed %d headers\n", h.count);
    for (int i = 0; i < h.count; i++)
        printf("  %s = %s\n", h.keys[i], h.vals[i]);
    return 0;
}
