# 06. NULL 역참조 — ':' 없는 줄에서 strchr 의 NULL 에 그대로 썼다

> HTTP 헤더 블록을 줄마다 key/value 로 나누는 프로그램이, `:` 가 없는 `"Connection"` 줄에서 SIGSEGV 로 죽었다.
> 원인은 `strchr(line, ':')` 이 돌려준 NULL 을 검사하지 않고 `*colon = '\0'` 로 곧장 쓴 것이다.

| 항목 | 내용 |
| --- | --- |
| 유형 | NULL 역참조 |
| 신호 | `SIGSEGV` |
| 환경 | x86-64 Linux · gcc `-g -O0 -fno-omit-frame-pointer` · gdb |

## 1. 증상

`"Key: Value"` 형식의 헤더 4줄(`Host` · `Accept` · `Connection` · `User-Agent`)을 `strtok` 으로 한 줄씩 자르고, 각 줄의 `:` 자리를 `'\0'` 으로 끊어 key / value 로 저장한 뒤 출력하는 프로그램이다.
아무것도 출력하지 못하고 `SIGSEGV` 로 죽는다.

## 2. gdb 역추적 — 소스를 열기 전에

```
(gdb) run
Program received signal SIGSEGV, Segmentation fault.
0x0000555555555224 in parse_headers (text=0x7fffffffeb80 "Host", h=0x7fffffffe970) at bug.c:52
52	        *colon = '\0';

(gdb) bt
#0  0x0000555555555224 in parse_headers (text=0x7fffffffeb80 "Host", h=0x7fffffffe970) at bug.c:52
#1  0x0000555555555385 in main () at bug.c:73

(gdb) x/i $pc
=> 0x555555555224 <parse_headers+76>:	movb   $0x0,(%rax)
(gdb) info registers rax rsi rdi
rax            0x0                 0
rsi            0x3a                58
rdi            0x7fffffffeb9e      140737488350110
(gdb) p $_siginfo._sifields._sigfault.si_addr
$1 = (void *) 0x0

(gdb) info locals
colon = 0x0
key = 0x7fffffffeb92 "Accept"
val = 0x7fffffffeb9a "*/*"
line = 0x7fffffffeb9e "Connection"
```

| 칸 | 알아낸 것 |
| --- | --- |
| 신호 | `SIGSEGV` — 건드리면 안 되는 주소를 건드렸다 |
| 죽은 명령 | `movb $0x0,(%rax)` — `%rax` 가 가리키는 곳에 1바이트 `0` 을 **쓰기** (`*colon = '\0'`) |
| 건드린 주소 · 구역 | `si_addr = 0x0`, `%rax = 0` — **진짜 NULL 에 쓰기** |
| 원인 프레임 | #0 `parse_headers` (bug.c:52) — 내 코드가 직접 역참조해서 `#0` 이 바로 내 함수다 |
| 원인 한 문장 | `:` 가 없는 줄 `"Connection"` 에서 `strchr` 이 NULL 을 돌려줬는데, 검사 없이 `*colon = '\0'` 로 썼다 |

gdb 화면에서 더 읽히는 것:

- **읽기가 아니라 쓰기다.** AT&T 문법에서 괄호가 **오른쪽(목적지)** 에 있으면 쓰기다 (`movb $0x0,(%rax)`). 주소 0 근처는 읽기도 쓰기도 안 되는 자리라 어느 쪽이든 SIGSEGV 다
- **`strchr` 호출 흔적이 레지스터에 남아 있다.** `%rdi` = `line`("Connection" 주소), `%rsi` = `0x3a` = `':'` — `strchr(line, ':')` 의 첫째 · 둘째 인자 그대로다
- **`key` · `val` 은 직전 줄 값이다** (`"Accept"`, `"*/*"`). 앞의 두 줄은 잘 처리됐고 세 번째 줄에서 죽었다는 흔적이다
- **`text` 가 `"Host"` 로만 보인다.** `strtok` 이 줄 끝 `'\n'` 을, `*colon = '\0'` 이 첫 줄의 `:` 를 원래 버퍼에 `'\0'` 으로 써 넣었기 때문이다. `text` 는 버퍼 시작이라 첫 `'\0'` 까지만 문자열로 보인다

## 3. 코드 흐름

**③ 값 버그** — 값이 만들어지는 곳 → 검사 없이 쓰이는 곳

```
strchr(line, ':')      ':' 없는 줄 "Connection" -> NULL      (bug.c:50)
      |
      v   검사 없이
*colon = '\0'          NULL 주소에 1바이트 쓰기                (bug.c:52)
      |
      v
SIGSEGV
```

## 4. 원인

| | 위치 |
| --- | --- |
| 죽은 곳 | `parse_headers` 의 `*colon = '\0'` (bug.c:52) |
| 진짜 원인 | `strchr` 의 반환값을 검사하지 않은 것 (bug.c:50 과 52 사이) |

**깨진 가정**: "헤더 줄에는 항상 `:` 가 있다" — 입력 네 줄 중 `"Connection"` 에는 없다. `strchr` 은 못 찾으면 NULL 을 돌려준다.

**첫 단서**: `#0` 이 내 함수이고, `x/i $pc` 가 `(%rax)` 를 건드리는데 `%rax = 0` 이다 → 그 레지스터에 담긴 포인터 변수(`colon = 0x0`)를 `info locals` 로 찾는다 → 그 포인터를 돌려준 함수(`strchr`)를 본다.

## 5. 해결

```c
char *colon = strchr(line, ':');
if (colon == NULL)          /* ':' 없는 줄은 건너뛴다 */
    continue;
*colon = '\0';
```

**왜 거기서 고쳤나**: NULL 이 처음 들어오는 곳(`strchr` 바로 다음 줄)에서 한 번 검사한다. `:` 없는 줄을 건너뛸지 오류로 멈출지는 파서가 정할 일인데(과제 TODO 도 둘 다 허용), 헤더 모양이 아닌 줄은 무시하기로 했다. 그래서 `Connection` 은 조용히 빠진다.

```
parsed 3 headers
  Host = example.com
  Accept = */*
  User-Agent = memdbg-cli
```

검증: 일반 실행 exit 0 · ASan+UBSan 메모리 오류 없음 · 누수 없음 · 미초기화 검사 통과.

## 6. 배운 점

- **05 와 같은 모양, 다른 죽는 곳.** 둘 다 "없음" 을 NULL 로 돌려주는 함수(`cfg_get` · `strchr`)의 반환값을 검사 없이 썼다. 05 는 NULL 을 libc(`strlen`)에 **넘겨서** libc 안(`#0 ??`)에서 죽었고, 06 은 내 코드가 **직접** `*colon` 에 써서 `#0` 이 내 함수다
- `x/i $pc` 로 읽기 · 쓰기를 구분한다: 괄호가 왼쪽(`mov (%rax),%rdx`)이면 읽기, 오른쪽(`movb $0x0,(%rax)`)이면 쓰기. 괄호 안 레지스터 값이 0 이면 NULL 역참조다
- `x/i` 가 지목한 레지스터가 **어느 변수인지**는 `info locals` 에서 같은 값을 가진 포인터를 찾으면 된다 (`%rax = 0` ↔ `colon = 0x0`)
- 건너뛰는 것도 결정이다 — 05 의 "없는 키는 빈 문자열" 처럼, 06 은 "헤더 모양이 아닌 줄은 무시" 를 골랐다. 입력을 조용히 버리는 게 맞는지는 프로그램이 정한다
- gdb 에서 libc 함수를 직접 불러 볼 수 있다: `p (char *)strchr(line, ':')`. 반환 타입을 캐스트해야 하고, 문자는 작은따옴표 `':'` 로 쓴다 (`":"` 는 문자열이라 주소가 넘어가 엉뚱한 결과가 나온다)
