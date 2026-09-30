# 07. Stack Use-After-Return — 이미 끝난 함수의 배열을 들고 나왔다

> 문자열을 줄로 쪼개 줄 주소들의 "뷰"를 돌려주는 프로그램이, 뷰를 읽는 순간 `0x4141414141414141` 을 주소로 따라가다 SIGSEGV 로 죽었다.
> 원인은 `split_lines` 가 **자기 지역 배열 `parts[]` 의 주소**를 뷰에 담아 내보낸 것 — 함수가 끝나자 그 자리는 주인 없는 땅이 됐고, 다음 함수(`warm_stack`)가 덮었다.

| 항목 | 내용 |
| --- | --- |
| 유형 | Stack Use-After-Return (지역 배열 주소가 함수 밖으로 탈출) |
| 신호 | `SIGSEGV` |
| 환경 | x86-64 Linux · gcc `-g -O0 -fno-omit-frame-pointer` · gdb |

## 1. 증상

`"alpha\nbeta\ngamma"` 를 `strtok` 으로 세 줄로 쪼개, 각 줄의 시작 주소들을 담은 뷰(`LineView`)를 만든다. `main` 이 뷰를 받아 줄 수와 각 줄 첫 글자의 합을 출력해야 한다 (`lines = 3, checksum = 298`).
아무것도 출력하지 못하고 `SIGSEGV` 로 죽는다.

## 2. gdb 역추적 — 소스를 열기 전에

```
(gdb) run
Program received signal SIGSEGV, Segmentation fault.
main () at bug.c:87
87	        checksum += (unsigned char)v.lines[i][0];

(gdb) bt
#0  main () at bug.c:87

(gdb) x/i $pc
=> 0x555555555335 <main+120>:	movzbl (%rax),%eax
(gdb) info registers rax rsp
rax            0x4141414141414141  4702111234474983745
rsp            0x7fffffffeb90      0x7fffffffeb90
(gdb) p $_siginfo._sifields._sigfault.si_addr
$1 = (void *) 0x0

(gdb) info locals
i = 0
text = "alpha\000beta\000gamma"
v = {lines = 0x7fffffffeb30, count = 3}
checksum = 0
```

| 칸 | 알아낸 것 |
| --- | --- |
| 신호 | `SIGSEGV` |
| 죽은 명령 | `movzbl (%rax),%eax` — `%rax` 가 가리키는 1바이트 읽기 = `v.lines[i][0]` |
| 건드린 주소 · 구역 | `%rax = 0x4141414141414141` — 주소로 쓸 수 없는 값. `si_addr` 은 `0x0` 으로 나오지만 **NULL 이 아니다** (01 과 같은 함정: 이런 모양의 주소는 커널이 `si_addr` 을 0 으로 보고한다) |
| 원인 프레임 | **bt 에 없다.** `bt` 는 `main` 하나뿐 — 원인인 `split_lines` 는 이미 리턴했다 |
| 원인 한 문장 | `split_lines` 가 지역 배열 `parts[]` 의 주소를 `LineView.lines` 에 담아 내보냈고, `warm_stack` 이 그 스택 자리를 덮었다 |

gdb 화면에서 더 읽히는 것:

- **포인터 자체는 멀쩡한 스택 주소다.** `v.lines = 0x7fffffffeb30` 은 `0x7fff…` 스택 구역이다. 그런데 그 안의 내용(`v.lines[0]`)이 쓰레기다 — 이 유형의 지문
- **`v.lines` 가 `main` 의 `%rsp` 보다 아래다.** `0x7fffffffeb30 < %rsp 0x7fffffffeb90`. 스택은 아래로 자라니, `%rsp` 아래는 **이미 끝난 함수들이 쓰던 자리**다. `main` 이 지금 들고 있으면 안 되는 주소다
- **줄 자체(`text`)는 멀쩡하다.** `text = "alpha\000beta\000gamma"` — `main` 의 배열이고, `strtok` 이 `'\n'` 을 `'\0'` 으로 바꿔 놓았을 뿐이다. 죽은 건 줄들이 아니라 **줄 주소를 모아 둔 배열**이다

## 3. 코드 흐름

**① 수명 버그** — 메모리 하나의 생애 (`split_lines` 의 지역 배열 `parts[8]`)

| 단계 | 어디서 | 무슨 일 |
| --- | --- | --- |
| 태어남 | `split_lines` (bug.c:56) | 지역 배열 `char *parts[8]` — `split_lines` 스택 프레임 안 |
| 복사해 감 | `view_set` (bug.c:64) | `out->lines = parts` — **배열 주소**가 `main` 의 `v` 로 나간다 |
| 죽음 | `split_lines` 리턴 | 프레임이 사라져 `parts` 자리는 주인 없는 땅 |
| 덮임 | `warm_stack` (bug.c:83) | 같은 깊이에서 불려 **같은 자리**에 `scratch[8]` 을 만들고 `0x4141…` 로 채운다 |
| 죽은 뒤 씀 | `main` checksum 루프 (bug.c:87) | `v.lines[0]` = `0x4141…` 을 주소로 읽음 → SIGSEGV |

### 실험 — 더 깊은 프레임에 배열을 두면? (9/21)

**가설**: `split_lines` 가 부르는 `helper` 안에 배열(`new_arr`)을 두면 `warm_stack` 과 안 겹쳐서 괜찮지 않을까?

**결과**: 통과했다. **그러나 운이었다.**

```
높은 주소
0x7fffffffe110 ┬─────────────────────────┐ ← main %rbp
               │        main 프레임       │
0x7fffffffe0d0 ┼─────────────────────────┤ ← main %rsp   ════ 이 아래는 주인 없는 땅
0x7fffffffe0c8 │  리턴 주소 (→ main)      │
0x7fffffffe0c0 ├─────────────────────────┤ ← split_lines %rbp ＝ warm_stack %rbp
0x7fffffffe0b0 │ ┌─────────────────────┐ │
               │ │ parts[8]  / scratch │ │ ← ★ 둘 다 0x...e070. 주소가 완전히 같다
0x7fffffffe070 │ └─────────────────────┘ │
0x7fffffffe060 │- - - - - - - - - - - - -│ ← warm_stack %rsp (여기까지만 내려옴)
0x7fffffffe050 ├─────────────────────────┤ ← split_lines %rsp
0x7fffffffe048 │  리턴 주소 (→ split)     │
0x7fffffffe040 ├─────────────────────────┤ ← helper %rbp
               │     ↕ 틈 0x30 = 48바이트 │   ← "운"의 크기
0x7fffffffe030 │ ┌─────────────────────┐ │
               │ │ new_arr[8] (64B)    │ │ ← out->lines 가 가리키는 곳
0x7fffffffdff0 │ └─────────────────────┘ │
0x7fffffffdfc0 └─────────────────────────┘ ← helper %rsp
```

| 관찰 | 뜻 |
| --- | --- |
| `parts` = `scratch` = `0x...e070` | 원래 버그가 100% 터지는 이유. 같은 깊이에서 불린 두 함수는 같은 자리를 쓴다 |
| `warm_stack` 은 `e060` 까지만 | `new_arr`(`e030` 아래)에 안 닿아서 통과 |
| 틈 48바이트 | `warm_stack` 에 지역변수 하나만 커져도 덮인다 |
| `new_arr` < main `%rsp` | **처음부터 죽은 땅.** 덮였는지와 무관하게 버그 |

더 가까운 위협: `main` 이 checksum 뒤에 부르는 `printf` 는 스택을 수백 바이트 쓴다. 순서만 바뀌면 `new_arr` 를 밟는다.

**판정 기준**: "덮였나" 가 아니라 **"쓰는 시점에 `%rsp` 보다 위인가."** 아래면 죽은 땅이다.

곁가지 버그: 처음 쓴 `helper` 에서 `new_arr[i] = *arr[i]` — 한 단계 더 들어가 `'a'`(0x61)를 포인터로 넣었다. 필요한 건 `arr[i]` 다 (포인터를 넘겨도 한 단계만 뚫린다).

## 4. 원인

| | 위치 |
| --- | --- |
| 죽은 곳 | `main` 의 checksum 루프 (bug.c:87) |
| 진짜 원인 | `split_lines` 가 `parts[]` 주소를 내보낸 것 (bug.c:56 · 64) |
| 거리 | **함수가 이미 사라짐** — bt 에 안 나온다 |

**깨진 가정**: "주소를 넘겼으니 받은 쪽이 계속 쓸 수 있다" — 지역 배열의 수명은 그 함수가 끝날 때까지다. 받는 쪽이 포인터를 **저장**해서 나중에 쓰면, 수명이 함수 밖까지 가야 한다.

**첫 단서**: 포인터 자체는 유효한 `0x7fff…`(스택)인데 **그 안의 내용이 쓰레기**다. 그 포인터가 `%rsp` 보다 아래면 이미 끝난 함수의 자리다. 함수 안에서 찍은 주소와 밖에서 찍은 주소가 **같으면** 이 유형이다.

## 5. 해결 — 배열의 주인을 바꾸는 네 가지

줄 자체(`text`)는 `main` 의 배열이라 안전하다. 문제는 **줄 주소를 모은 배열의 수명**뿐이다. 과제 TODO 는 "호출자가 소유하는 저장소(배열/힙)에 결과를 채우거나, 힙에 할당해 수명을 넘기라" 고 한다. 저장소 폴더에 네 가지로 풀어 두었다 (넷 다 `lines = 3, checksum = 298`).

| 파일 | 배열의 주인 | 수명 | 해제 |
| --- | --- | --- | --- |
| `mybug.c` (제출) | 구조체 `LineView` 안 — `v` 는 `main` 지역 | `main` 이 끝날 때까지 | 없음 |
| `mybug1.c` | `main` 의 지역 배열 | `main` 이 끝날 때까지 | 없음 |
| `mybug2.c` | `main` 이 `malloc` 한 힙 | `main` 이 `free` 할 때까지 | `main` (잡은 쪽) |
| `mybug3.c` | `split_lines` 가 `malloc` 한 힙 | 받은 쪽이 `free` 할 때까지 | `main` (넘겨받은 쪽) |

### mybug.c — 구조체 안에 배열 (제출)

```c
typedef struct {
    char *lines[MAX_LINES];   /* char ** → 배열. 칸이 구조체 안에 있다 */
    int count;
} LineView;

static void view_set(LineView *out, char *arr) {
    out->lines[out->count++] = arr;
}

static void split_lines(LineView *out, char *text) {
    for (char *ln = strtok(text, "\n"); ln && out->count < MAX_LINES; ln = strtok(NULL, "\n"))
        view_set(out, ln);
}

LineView v = {.count = 0};     /* main */
split_lines(&v, text);
```

**왜 안전한가**: 저장소가 `v` 안에 있고 `v` 는 `main` 프레임이다 — 쓰는 시점에 `%rsp` 보다 위. `warm_stack` 은 `main` 아래에만 프레임을 잡는다.

**`char **` 로는 안 되는 이유**: 포인터 1개(8B)뿐이라 칸이 없다. 초기화 안 된 `v.lines` 로 쓰면 쓰레기 주소에 쓴다. 그리고 배열로 바꾸면 `out->lines = arr` 는 **컴파일 에러** — 배열은 대입이 안 된다. 그래서 원래 `view_set(out, parts, n)` 모양이 필요 없어진다.

### mybug1.c — 호출자가 소유하는 배열

```c
/*
 * 07 풀이 1 — 호출자가 소유하는 '배열'에 결과를 채운다
 *   main 이 char *parts[MAX_LINES] 를 갖고, split_lines 는 그 배열을 받아 채우기만 한다.
 *   배열의 수명 = main 의 스택 프레임 → main 이 v.lines 를 쓰는 동안 살아 있다.
 */
static void split_lines(LineView *out, char **parts, char *text) {
    int n = 0;
    for (char *ln = strtok(text, "\n"); ln && n < MAX_LINES; ln = strtok(NULL, "\n"))
        parts[n++] = ln;
    view_set(out, parts, n);      /* parts 는 호출자의 배열 — 이 함수가 끝나도 살아 있다 */
}

char *parts[MAX_LINES];              /* 호출자(main)가 소유하는 배열 */
LineView v;
split_lines(&v, parts, text);
```

### mybug2.c — 호출자가 소유하는 힙

```c
/*
 * 07 풀이 2 — 호출자가 소유하는 '힙'에 결과를 채운다
 *   main 이 malloc 으로 배열을 잡아 넘기고, split_lines 는 채우기만 한다.
 *   다 쓴 뒤 해제도 잡은 쪽인 main 이 한다.
 */
char **parts = malloc(MAX_LINES * sizeof *parts);   /* 호출자(main)가 힙에 잡는다 */
if (!parts) { perror("malloc"); return 1; }
LineView v;
split_lines(&v, parts, text);        /* split_lines 는 풀이 1 과 같다 */
...
free(parts);                         /* 잡은 쪽(main)이 해제한다 */
```

### mybug3.c — 힙에 할당해 수명을 넘긴다

```c
/*
 * 07 풀이 3 — 힙에 할당해 수명을 넘긴다
 *   split_lines 가 배열을 malloc 해서 뷰에 담아 돌려준다. 배열은 함수가 끝나도 살아 있고,
 *   해제 책임은 뷰를 받은 호출자(main)에게 넘어간다 → main 이 view_free 로 해제한다.
 */
static void split_lines(LineView *out, char *text) {
    char **parts = malloc(MAX_LINES * sizeof *parts);   /* 힙에 잡는다 — 함수가 끝나도 살아 있다 */
    if (!parts) { perror("malloc"); exit(1); }
    int n = 0;
    for (char *ln = strtok(text, "\n"); ln && n < MAX_LINES; ln = strtok(NULL, "\n"))
        parts[n++] = ln;
    view_set(out, parts, n);      /* 해제 책임은 뷰를 받은 호출자에게 넘어간다 */
}

/* split_lines 가 넘긴 배열을 해제한다 — 호출자가 다 쓴 뒤 부른다 */
static void view_free(LineView *v) {
    free(v->lines);
    v->lines = NULL;
    v->count = 0;
}
```

검증: 네 파일 모두 일반 실행 exit 0 · ASan+UBSan 메모리 오류 없음 · 누수 없음 · 미초기화 검사 통과.

### 저장소 고르는 법

| 방법 | 좋은 점 | 대가 |
| --- | --- | --- |
| 호출자 배열 (`mybug1`) | 빠름, free 없음 | 크기 고정, 호출자 수명에 묶임 |
| **구조체 안 배열** (`mybug`) | 인자가 안 늘고 저장소가 딸려 온다 | 크기 고정 |
| 힙 (`mybug2` · `mybug3`) | 크기 자유, 어떤 프레임보다 오래 산다 | free 책임 — 누가 할지 정해야 한다 |
| `static` 지역 | 리턴해도 안 죽는다 | 호출마다 공유 → 덮인다 |

질문 둘: **줄 수가 미리 정해져 있나? 결과가 얼마나 멀리 가야 하나?** 기준은 `main` 이 아니라 **결과를 쓰는 쪽의 프레임**이다.

### 1차 시도(9/19, malloc)에서 걸린 네 가지

| # | 실수 | 교훈 |
| ---: | --- | --- |
| 1 | `free` 루프가 `v.lines[i]`(= `text` 안 **스택 주소**)를 해제 | `strtok` 은 할당하지 않는다. malloc 은 **배열 1개** |
| 2 | `copy = "";` | 읽기 전용 리터럴에 `memcpy` → SIGSEGV. **05 와 같은 반사신경** |
| 3 | `char *copy` 를 `char **` 자리에 전달 | 바이트는 맞고 타입만 틀림 — 제일 위험한 종류 |
| 4 | `sizeof parts`(64) / `copy[len]` 널 종료 | 문자열 복사 감각의 잔재. 포인터 배열엔 널 종료가 없다 |

**핵심(수명 판정)은 맞췄다.** 틀린 건 전부 malloc/free 손버릇이다.

### 리뷰에서 나온 것 (9/21)

| | |
| --- | --- |
| 줄 0개면 `count` 가 쓰레기 | 채우는 함수는 시작할 때 `count = 0` (지금 `mybug.c` 는 `main` 의 `{.count = 0}` 이 대신한다) |
| `lines[n] = NULL` 직후 덮어씀 | 죽은 줄 |
| `n` 을 호출자와 `view_set` 이 나눠 관리 | 개수 · 경계 검사를 한 곳에 |
| 틀린 주석(`-Wdangling 회피`, TODO) | 틀린 주석은 없는 주석보다 나쁘다 |

## 6. 배운 점

- **수명 버그는 크래시 지점을 봐선 안 보인다.** 원인 함수는 이미 리턴해서 bt 에도 없다. "누가 얼마나 오래 들고 있나" 를 물어야 찾는다
- 이 버그는 **가끔 잘 돌아간다** — 스택이 아직 안 덮였으면. `printf` 하나 넣었더니 죽는 현상의 정체다
- **판정 기준은 "덮였나" 가 아니라 "쓰는 시점에 `%rsp` 보다 위인가"** — 아래면 이미 끝난 함수의 자리다. 이번 크래시도 `v.lines(0x…eb30) < %rsp(0x…eb90)` 였다
- `si_addr = 0x0` 이라고 NULL 로 단정하지 않는다. 01 · 07 은 `%rax` 에 주소로 쓸 수 없는 값(`"STATUS: "` · `0x4141…`)이 들어 있었다. `x/i` 가 가리킨 레지스터 값을 본다
- 받는 쪽이 포인터를 **즉시 복사**하나 **저장**하나를 먼저 본다. 저장하면 가리키는 저장소의 주인과 수명을 정해야 한다 — 네 가지 풀이는 결국 "배열의 주인을 누구로, 얼마나 오래" 를 고른 것이다
- **free 의 개수는 malloc 의 개수와 같아야 한다**

### 리턴하면 지역 배열의 값이 사라지나?

사라지지 않는다. `ret` 은 스택의 바이트를 지우지 않고 **`%rsp` 를 위로 올릴 뿐**이다. 문제는 값이 사라지는 게 아니라, **그 자리가 다음 함수 호출에게 다시 내어진다**는 것이다.

```
return 직전 (split_lines 안)             return 직후 (main 으로 돌아옴)

  +------------------+                     +------------------+
  | main frame       |                     | main frame       |
  +------------------+                     +------------------+  <- %rsp (main 의 바닥)
  | split_lines      |                     | parts[8]         |  <- 값은 그대로 남아 있다
  |   parts[8]       |                     |   (free)         |     하지만 %rsp 아래 = 주인 없음
  +------------------+  <- %rsp            +------------------+
```

- 07 에서 `parts` 와 `warm_stack` 의 `scratch` 가 **완전히 같은 주소**였다 — 같은 깊이에서 불린 다음 함수가 그 자리를 그대로 받아 간다
- 덮는 쪽은 "다른 함수" 만이 아니다. 다음에 부르는 무엇이든 — `printf` 같은 라이브러리 함수도 그 자리를 쓴다. 그래서 이 버그는 가끔 잘 돌아가다가 `printf` 한 줄에 죽는다
- **안 덮였어도 이미 틀린 코드다.** C 표준으로는 함수가 끝나는 순간 지역 배열의 수명이 끝나고, 수명이 끝난 걸 읽는 것 자체가 정의되지 않은 동작이다. 덮이는 건 버그가 **드러나는 방식**일 뿐이다

**`return` 은 스택의 자동 `free` 다** — 01(힙)과 나란히 보면 같은 모양이다.

| | 01 UAF (힙) | 07 UAR (스택) |
| --- | --- | --- |
| 자리를 돌려주는 순간 | `free` (사람이 부른다) | `return` (자동) |
| 값은? | 그대로 남는다 | 그대로 남는다 |
| 다시 가져가는 쪽 | 같은 크기 `malloc` (tcache) | 다음 함수 호출의 프레임 |
| 결과 | 문자열이 vtable 자리를 덮었다 | `0x4141…` 이 `parts` 자리를 덮었다 |

둘 다 같은 질문이다: **주인이 자리를 돌려준 뒤에도 누가 그 주소를 들고 있나?**

### %rsp 위와 아래

| | `%rsp` 위 | `%rsp` 아래 |
| --- | --- | --- |
| 누구 자리인가 | **아직 끝나지 않은 함수들**의 프레임 (`main` 과 그 호출자들) | 끝난 함수들이 쓰던 **빈자리** |
| 새 함수를 부르면 | 새 프레임은 항상 **아래에** 생긴다. 위는 다시 쓰이지 않는다 | 다음 호출이 그 자리를 **그대로** 받아 간다 |
| 결론 | 주인이 있다 → 주인이 끝날 때까지 안전 | 주인이 없다 → **언제든, 무엇이든** 덮을 수 있다 |

> **`%rsp` 위는 살아 있는 함수의 자리라서, 주인이 끝날 때까지 다른 호출이 자기 자리로 가져가지 않는다. `%rsp` 아래는 주인 없는 자리라서, 다음 호출 누구든 언제든 가져가 덮을 수 있다.**

`%rsp` 위라고 **쓰기가 불가능한 건 아니다** — 새 프레임으로 다시 쓰이지 않을 뿐이다.

- **주인이 허락한 쓰기**: `mybug.c` 의 `split_lines` 는 `main` 의 `v`(위쪽)에 직접 쓴다. `main` 이 `&v` 를 넘겨 "여기에 채워 줘" 라고 허락한 것이라 정상이다
- **넘쳐서 쓰는 버그**: 02 에서 `build_pascal` 은 `main` 의 배열 주소를 받아 쓰다가 넘쳐, 위쪽의 카나리 · 저장된 rbp · 리턴 주소까지 덮었다

참고: x86-64 에는 `%rsp` 바로 아래 128바이트 **red zone** 이 있어, **지금 실행 중인** 함수가 다른 함수를 부르지 않을 때 잠깐 써도 된다. 그래도 이미 끝난 함수의 몫은 아니라 위 결론은 그대로다.
