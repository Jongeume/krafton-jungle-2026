# 05. NULL 반환 미확인 — 없는 키의 NULL 을 그대로 strlen 에 넘겼다

> 설정값으로 URL 템플릿을 채우는 프로그램이, 설정에 없는 `${path}` 를 만나는 순간 libc 안에서 SIGSEGV 로 죽었다.
> 원인은 `cfg_get` 이 없는 키에 돌려준 NULL 을 `expand` 가 검사 없이 `strlen` 에 넘긴 것이다.

| 항목 | 내용 |
| --- | --- |
| 유형 | NULL 반환 미확인 역참조 |
| 신호 | `SIGSEGV` |
| 환경 | x86-64 Linux · gcc `-g -O0 -fno-omit-frame-pointer` · gdb |

## 1. 증상

key=value 설정(`host`, `port`)으로 템플릿 `"http://${host}:${port}/${path}/index.html"` 의 `${키}` 자리를 채워 URL 을 출력하는 프로그램이다.
아무것도 출력하지 못하고 `SIGSEGV` 로 죽는다.

## 2. gdb 역추적 — 소스를 열기 전에

```
(gdb) run
Program received signal SIGSEGV, Segmentation fault.
0x00007ffff7f30add in ?? () from /lib/x86_64-linux-gnu/libc.so.6

(gdb) bt
#0  0x00007ffff7f30add in ?? () from /lib/x86_64-linux-gnu/libc.so.6
#1  0x00005555555553b8 in expand (c=0x7fffffffe9b0, tmpl=0x555555556028 "http://${host}:${port}/${path}/index.html", out=0x7fffffffeac0 "http://example.com:8080/\230\353\377\377\377\177", outcap=256) at bug.c:68
#2  0x000055555555550d in main () at bug.c:102

(gdb) x/i $pc
=> 0x7ffff7f30add:	vpcmpeqb (%rdi),%ymm0,%ymm1
(gdb) info registers rdi
rdi            0x0                 0
(gdb) p $_siginfo._sifields._sigfault.si_addr
$1 = (void *) 0x0

(gdb) frame 1
#1  expand (...) at bug.c:68
68	            size_t vl = strlen(v);
(gdb) info locals
key = "path\000..."
v = 0x0
o = 24
```

| 칸 | 알아낸 것 |
| --- | --- |
| 신호 | `SIGSEGV` — 건드리면 안 되는 주소를 읽었다 |
| 죽은 명령 | libc 안 `vpcmpeqb (%rdi),%ymm0,%ymm1` — `%rdi` 가 가리키는 곳을 읽다가. 함수 이름은 `??` 로 나오지만 `frame 1` 의 줄이 `strlen(v)` 라 `strlen` 안이다 |
| 건드린 주소 · 구역 | `si_addr = 0x0`, `%rdi = 0` — **진짜 NULL** |
| 원인 프레임 | #1 `expand` (bug.c:68). `#0` 이 내 코드가 아니라 `frame 1` 로 올라와야 보인다. `key = "path"`, `v = 0x0` |
| 원인 한 문장 | 설정에 없는 키 `path` 에 `cfg_get` 이 NULL 을 돌려줬고, `expand` 가 검사 없이 `strlen` 에 넘겼다 |

**왜 `%rdi` 인가**: x86-64 에서 함수의 **첫 번째 인자는 `%rdi`** 로 넘어간다(CSAPP 3.7.3). `strlen(v)` 의 `v` 가 `%rdi` 에 들어갔고, 그 값이 0 이라 주소 0 을 읽다가 죽었다.

**01 과 다른 점**: 01 도 `si_addr` 이 `0x0` 이었지만 NULL 이 아니었다(가리킨 레지스터에 문자열 `"STATUS: "` 가 들어 있었다). 여기는 가리킨 레지스터(`%rdi`)가 **정말 0** 이다. `si_addr` 만 보지 말고 `x/i $pc` 가 가리킨 레지스터 값을 본다.

## 3. 코드 흐름

**③ 값 버그** — 값이 만들어지는 곳 → 검사 없이 쓰이는 곳

```
cfg_get(c, "path")        없는 키 → return NULL          (bug.c:52)
      |
      v   검사 없이
strlen(v)                 v = NULL → libc 가 주소 0 을 읽음   (bug.c:68)
      |
      v
SIGSEGV
```

## 4. 원인

| | 위치 |
| --- | --- |
| 죽은 곳 | libc `strlen` 안 (`#0 ??`) |
| 진짜 원인 | `expand` 가 `cfg_get` 의 반환값을 검사하지 않고 `strlen` 에 넘긴 것 (bug.c:67~68) |

**깨진 가정**: "`cfg_get` 은 항상 문자열을 돌려준다" — `cfg_get` 은 없는 키에 NULL 을 돌려주도록 만들어져 있다(bug.c:52). NULL 은 문자열이 아니라 "없음" 이라는 신호다.

**첫 단서**: `#0` 이 libc 인데 `si_addr` 이 진짜 `0x0` 이다 → libc 에 NULL 을 넘긴 내 줄을 찾는다 → `frame 1` → `info locals` 에서 값이 `0x0` 인 포인터(`v`).

## 5. 해결

```c
const char *v = cfg_get(c, key);
if (v == NULL)          /* 없는 키는 여기서 빈 문자열로 */
    v = "";
size_t vl = strlen(v);
```

**왜 거기서 고쳤나**: `cfg_get` 의 약속(없는 키 → NULL)은 그대로 두고, 그 NULL 을 **받아 쓰는 쪽**인 `expand` 가 어떻게 할지 정했다. 템플릿을 채우는 `expand` 는 "없는 키는 비워 둔다" 고 정했다.

```
url = http://example.com:8080//index.html
```

검증: 일반 실행 exit 0 · ASan+UBSan 메모리 오류 없음 · 누수 없음 · 미초기화 검사 통과.

## 6. 배운 점

- libc 함수 안에서 죽고 `si_addr` 이 `0x0` 이면, 내가 그 함수에 NULL 을 넘겼는지부터 본다. 첫 인자는 `%rdi` 에 있다
- 원칙은 **"쓰기 전에 검사"** — NULL 을 못 만들게 막는 게 아니라, NULL 을 만나는 쪽이 검사한다

### NULL 을 넘기면 안 되는 libc 함수 — strlen · strcmp · memcpy

먼저 세 함수가 하는 일이다 (모두 `<string.h>`).

| 함수 | 하는 일 | 05 에서 쓴 곳 |
| --- | --- | --- |
| `size_t strlen(const char *s)` | `s` 부터 `'\0'` 을 만날 때까지 글자 수를 센다. `'\0'` 은 세지 않는다 (`strlen("path")` = 4) | `expand` 에서 값의 길이 `strlen(v)` (bug.c:68) |
| `int strcmp(const char *a, const char *b)` | 두 문자열을 앞에서부터 한 글자씩 비교한다. 같으면 0, `a` 가 사전순으로 앞이면 음수, 뒤면 양수 | `cfg_get` 에서 키 찾기 (bug.c:51) |
| `void *memcpy(void *dst, const void *src, size_t n)` | `src` 에서 정확히 `n` 바이트를 `dst` 로 복사한다. `'\0'` 을 보지 않고, 두 영역이 겹치면 안 된다 | `expand` 에서 키 · 값 복사 (bug.c:64 · 69) |

셋 다 받은 주소에서 **바로 읽기 시작한다**는 게 공통점이다.

`str*` · `mem*` 함수는 받은 주소가 유효하다고 **믿고 바로 읽는다.** NULL 인지 검사해 주지 않으니, NULL 을 넘기면 주소 0 을 읽다가 대개 SIGSEGV 로 죽는다. C 표준으로는 **정의되지 않은 동작(UB)** 이라, 크래시는 보장이 아니라 가장 흔한 결과다.

| 함수 | NULL 을 넘기면 | 근거 |
| --- | --- | --- |
| `strlen(NULL)` | 첫 글자를 읽으려다 SIGSEGV | **이 문제에서 봤다** (`vpcmpeqb (%rdi)`, `%rdi = 0`) |
| `strcmp(NULL, s)` · `strcmp(s, NULL)` | 첫 글자를 비교하려다 SIGSEGV | C 표준상 UB — glibc 는 검사하지 않는다 |
| `memcpy(dst, NULL, n)` | `n > 0` 이면 읽다가 SIGSEGV. **`n == 0` 이어도 표준상 UB** | 〃 |
| `strcpy` · `strcat` · `strchr` 등 | 같다 | 〃 |
| 반대로 `free(NULL)` | 아무 일도 안 한다 | 표준이 따로 정해 둔 예외 (04) |

- 05 에서도 `strlen` 이 버텼다면 바로 다음 줄 `memcpy(out + o, v, vl)` 에 같은 NULL 이 들어갔다. 검사는 **NULL 이 처음 들어오는 곳** 한 번이면 된다 (`cfg_get` 을 부른 바로 다음 줄)
- 어느 인자가 NULL 인지는 레지스터로 본다: 첫째 `%rdi` · 둘째 `%rsi` · 셋째 `%rdx`. `strcmp(a, b)` 면 `a` 는 `%rdi`, `b` 는 `%rsi` · `memcpy(dst, src, n)` 이면 `src` 는 `%rsi`

#### NULL 을 돌려주는 함수 — strchr · strtok

앞의 셋은 NULL 을 **넘기면** 죽는다. 이 둘은 NULL 을 **돌려주기도** 하고, `strtok` 은 NULL 을 **일부러 넘기기도** 한다.

| 함수 | 하는 일 | NULL 과 얽히는 곳 |
| --- | --- | --- |
| `char *strchr(const char *s, int c)` | `s` 에서 문자 `c` 가 **처음 나오는 자리의 주소**를 돌려준다 | 못 찾으면 **NULL 을 돌려준다** — 검사 없이 쓰면 NULL 역참조 |
| `char *strtok(char *s, const char *delim)` (참고 — 05 에는 없다) | 문자열을 `delim` 의 글자들로 잘라 토큰을 **하나씩** 돌려준다. 처음엔 문자열을, 이어서 자를 땐 **`NULL`** 을 넘긴다 (어디까지 잘랐는지 함수 안에 기억) | 첫 인자 `NULL` 은 "이어서" 라는 뜻 — **넘겨도 되는 드문 경우**. 토큰이 없으면 **NULL 을 돌려준다** |

05 에서 `strchr` 의 NULL 은 제대로 검사하고 있었다. 같은 함수 안에서 `strchr` 은 검사하고 `cfg_get` 은 안 한 것이 이 버그다.

```c
const char *end = strchr(p, '}');     /* bug.c:59 */
if (!end) break;                      /* 닫는 } 가 없으면 멈춘다 */
```

| 함수 | NULL 을 넘기면 | NULL 을 돌려받을 때 |
| --- | --- | --- |
| `strlen` · `strcmp` · `memcpy` | 죽는다 (UB) | 돌려주지 않는다 |
| `strchr` | 죽는다 (UB) | **못 찾았을 때 → 검사 필수** |
| `strtok` | 첫 인자 `NULL` 은 **"이어서" 라는 뜻 (정상)** | **토큰이 없을 때 → 검사 필수** |

`strtok` 은 NULL 말고도 함정이 둘 더 있다. 구분자 자리에 `'\0'` 을 써 넣어 **원본 문자열을 고치므로** 문자열 리터럴(읽기 전용)을 넘기면 쓰다가 SIGSEGV 가 나고(배열 `char buf[]` 에 담아 넘긴다), 위치를 함수 안에 기억하므로 두 문자열을 번갈아 자르면 꼬인다(`strtok_r` 은 위치를 직접 들고 다닌다).

### cfg_get 이 NULL 대신 빈 문자열을 돌려줬어야 하나?

처음 풀 때(9/19)는 실제로 그렇게 고쳤다 — `cfg_get` 의 `return NULL;` 을 `return "";` 로. 크래시는 사라지지만 의도와 다르다.

- **약속이 바뀐다.** `cfg_get` 이 `""` 를 돌려주면 "키가 없다" 와 "값이 빈 문자열이다" 를 구분할 수 없다. 없는 키를 **오류로 처리하고 싶은** 다른 호출자는 그 선택지를 잃는다
- NULL 은 버그가 아니라 **"없음" 이라는 정보**다. 그 정보를 받아 어떻게 할지는 쓰는 쪽이 정한다 — 01 이 "누가 해제할 권한이 있나" 였다면, 05 는 **"누가 결정할 권한이 있나"** 다
- 쓰는 쪽에서 `""` 로 채우는 것도 **결정**이다. 위 출력처럼 path 자리가 비어 `//` 가 생긴다. path 가 꼭 있어야 하는 URL 이라면 조용히 채우기보다 "없는 키: path" 로 알리고 멈추는 게 맞을 수 있다 (과제 TODO 도 "기본값(\"\") 또는 명시적 오류" 둘 다 허용한다)
- `""` 는 NULL 의 대체재가 아니다 — 읽기 전용이고 길이가 0 인 **진짜 문자열**이다. 없음을 없음으로 전하지 못한다
