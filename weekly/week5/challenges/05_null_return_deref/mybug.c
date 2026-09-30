#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_KV 16
typedef struct
{
    const char *keys[MAX_KV];
    const char *vals[MAX_KV];
    int n;
} Config;

static void cfg_set(Config *c, const char *k, const char *v)
{
    if (c->n < MAX_KV)
    {
        c->keys[c->n] = k;
        c->vals[c->n] = v;
        c->n++;
    }
}

static const char *cfg_get(const Config *c, const char *k)
{

    // n = 2
    for (int i = 0; i < c->n; i++)
        if (strcmp(c->keys[i], k) == 0)
            return c->vals[i];

    return NULL; /* 없는 키 → NULL */
}

static void expand(const Config *c, const char *tmpl, char *out, size_t outcap)
{
    size_t o = 0;
    for (const char *p = tmpl; *p;)
    {
        if (p[0] == '$' && p[1] == '{')
        {
            const char *end = strchr(p, '}');
            if (!end)
                break;
            char key[32];
            size_t kl = (size_t)(end - (p + 2));
            if (kl >= sizeof key)
                kl = sizeof key - 1;
            memcpy(key, p + 2, kl);
            key[kl] = '\0';

            const char *v = cfg_get(c, key);

            if (v == NULL)
            {
                v = "";
            }

            size_t vl = strlen(v);
            if (o + vl < outcap)
            {
                memcpy(out + o, v, vl);
                o += vl;
            }
            p = end + 1;
        }
        else
        {
            if (o + 1 < outcap)
                out[o++] = *p;
            p++;
        }
    }
    out[o] = '\0';
}

int main(void)
{

    Config cfg = {.n = 0};
    cfg_set(&cfg, "host", "example.com");

    cfg_set(&cfg, "port", "8080");

    const char *tmpl = "http://${host}:${port}/${path}/index.html";
    char out[256];

    expand(&cfg, tmpl, out, sizeof out); /* ${path} 치환 시 NULL 역참조 → 크래시 */

    printf("url = %s\n", out);
    return 0;
}