
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_LINES 8
typedef struct
{
    // char **lines; /* 줄 포인터들의 '배열'을 가리킨다 = 칸들의 주소 */
    char *lines[MAX_LINES];
    int count;
} LineView;

static void view_set(LineView *out, char *arr)
{
    out->lines[out->count++] = arr;
}

static void split_lines(LineView *out, char *text)
{
    // char *parts[MAX_LINES];
    // int n = 0;
    /* strtok는 새로 할당하지 않고, 넘겨받은 문자열 내부의 주소를 돌려준다.
     * 따라서, strtok은 원본 버퍼를 제자리에서 수정한다.
     */
    for (char *ln = strtok(text, "\n"); ln && out->count < MAX_LINES; ln = strtok(NULL, "\n"))
        view_set(out, ln);

    // parts[n++] = ln;
    /* TODO 상기 코드를 수정하여 결과를 호출자가 준 out 에 직접 채운다(값 반환 아님, 지역 주소 반환 아님). */
}
/* split_lines 가 쓰던 스택 프레임을, 같은 모양(char*[8])의 지역 배열로 덮는다.
   무효가 된 parts[] 자리에 '그럴듯한 쓰레기 포인터'가 들어차게 만든다. */
static void warm_stack(void)
{
    char *scratch[MAX_LINES];
    for (int i = 0; i < MAX_LINES; i++)
        scratch[i] = (char *)0x4141414141414141ULL; /* 매핑되지 않은 주소 */
    __asm__ volatile("" ::"r"(scratch) : "memory"); /* 최적화 제거 방지 */
}

int main(void)
{
    char text[] = "alpha\nbeta\ngamma";

    LineView v = {.count = 0};
    split_lines(&v, text);
    warm_stack();

    long checksum = 0;
    for (int i = 0; i < v.count; i++)
        checksum += (unsigned char)v.lines[i][0];

    printf("lines = %d, checksum = %ld\n", v.count, checksum);

    return 0;
}
