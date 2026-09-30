#include <stdio.h>
#include <stdlib.h>

typedef struct
{
    int *data;
    /* [Thinking Point]
     * 개수/크기를 담는 len, cap 을 왜 int 가 아니라 size_t 로 선언할까?
     *   tip 1. size_t 는 "이 플랫폼에서 표현 가능한 가장 큰 객체 크기"를 담도록 만든
     *          부호 없는(unsigned) 정수 타입이다. malloc/sizeof/strlen 의 타입도 size_t 다.
     *   tip 2. int 는 보통 32비트라 약 21억(2^31-1)에서 넘치고, 음수도 가능하다.
     *          원소가 그보다 많아지거나 cap*sizeof(int) 계산이 커지면 int 는 오버플로된다.
     *   생각해보기: 크기를 int 로 두면 어떤 버그가 생길 수 있을까?
     */
    size_t len;
    size_t cap;
} IntList;

static void list_init(IntList *l)
{
    l->cap = 8;
    l->len = 0;
    l->data = malloc(l->cap * sizeof(int));
    if (!l->data)
    {
        perror("malloc");
        exit(1);
    }
}

static void list_ensure(IntList *l, size_t need)
{
    if (need <= l->cap)
        return;

    size_t newcap = l->cap ? l->cap * 2 : 8;
    while (newcap < need)
        newcap *= 2;

    // realloc 할 때 newcap으로 사이즈를 적용안했음.
    int *p = realloc(l->data, newcap * sizeof(int));
    if (!p)
    {
        perror("realloc");
        free(l->data);
        exit(1);
    }

    l->data = p;
    l->cap = newcap;
}

static void list_push(IntList *l, int x)
{
    if (l->len == l->cap)
        list_ensure(l, l->cap + 1);
    l->data[l->len++] = x;
}

static long long list_sum(const IntList *l)
{
    long long s = 0;
    for (size_t i = 0; i < l->len; i++)
        s += l->data[i];
    return s;
}

static void list_free(IntList *l)
{
    free(l->data);
    l->data = NULL;
    l->len = l->cap = 0;
}

int main(void)
{
    IntList l;
    list_init(&l);

    const int N = 2000000;
    for (int i = 0; i < N; i++)
    {
        list_push(&l, i % 100);
    }

    printf("len=%zu cap=%zu sum=%lld\n", l.len, l.cap, list_sum(&l));
    list_free(&l);
    return 0;
}
