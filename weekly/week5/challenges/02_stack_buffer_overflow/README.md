# 02. Stack Buffer Overflow — 출력은 다 끝났는데 return 에서 죽었다

> 파스칼의 삼각형을 스택 배열에 채우는 프로그램이 출력을 전부 마친 뒤, `main` 이 끝나는 순간 `stack smashing detected` 로 죽었다.
> 원인은 행 루프 `i <= rows` 의 off-by-one — 없는 14번 행을 배열 끝 너머에 썼다.

| 항목 | 내용 |
| --- | --- |
| 유형 | Stack Buffer Overflow (off-by-one) |
| 신호 | `SIGABRT` (`*** stack smashing detected ***: terminated`) |
| 환경 | x86-64 Linux · gcc `-g -O0 -fno-omit-frame-pointer` · gdb |

## 1. 증상

파스칼의 삼각형 14행(0~13행)을 스택 위 1차원 배열 `int tri[105]` 에 채우고, 행마다 합(2^i)을 출력하는 프로그램이다.
출력은 마지막 `SIZE = 105` 까지 전부 정상으로 나온다. **그다음 `main` 이 끝나는 순간** `SIGABRT` 로 죽는다.

## 2. gdb 역추적 — 소스를 열기 전에

```
(gdb) run
row  0: 1   (sum=1)
...
row 13: 1 13 78 286 715 1287 1716 1716 1287 715 286 78 13 1   (sum=8192)
SIZE = 105
*** stack smashing detected ***: terminated

Program received signal SIGABRT, Aborted.

(gdb) bt
#0  0x00007ffff7e43c0c in pthread_kill () from /lib/x86_64-linux-gnu/libc.so.6
#1  0x00007ffff7dea27e in raise () from /lib/x86_64-linux-gnu/libc.so.6
#2  0x00007ffff7dcd8ff in abort () from /lib/x86_64-linux-gnu/libc.so.6
#3  0x00007ffff7dce7b6 in ?? () from /lib/x86_64-linux-gnu/libc.so.6
#4  0x00007ffff7edbea9 in __fortify_fail () from /lib/x86_64-linux-gnu/libc.so.6
#5  0x00007ffff7edd134 in __stack_chk_fail () from /lib/x86_64-linux-gnu/libc.so.6
#6  0x0000555555555428 in main () at bug.c:112

(gdb) x/i $pc
=> 0x7ffff7e43c0c <pthread_kill+284>:   mov    %eax,%r14d
```

| 칸 | 알아낸 것 |
| --- | --- |
| 신호 | `SIGABRT` — 프로그램이 스스로 `abort()` 를 불렀다 |
| 죽은 명령 | `__stack_chk_fail` 이 `abort` 를 불렀다 + 터미널 `*** stack smashing detected ***`. `x/i $pc` 는 libc `pthread_kill` 안이라 단서가 아니다 |
| 건드린 주소 · 구역 | 해당 없음 (SIGABRT) |
| 원인 프레임 | **bt에 없음.** 배열을 넘쳐 쓴 `build_pascal` 은 이미 리턴했다. bt의 `main` (bug.c:112) 은 닫는 `}` — 검사에 걸린 곳이다 |
| 원인 한 문장 | 행 루프가 한 번 더 돌아 `tri` 끝 너머의 스택을 덮었고, `main` 이 끝날 때 카나리 검사에 걸렸다 |

## 3. 코드 흐름

**② 경계 버그** — 두 숫자 비교

| 크기가 정해지는 곳 | 인덱스가 커지는 곳 | 어긋나는 순간 |
| --- | --- | --- |
| `SIZE = ROWS*(ROWS+1)/2 = 105` — 0~13행만 담는 크기 | `idx = i*(i+1)/2 + j`, 행 루프는 `i <= rows` | `i == 14` 에서 `idx = 105 + j` (j = 0~14) → `tri[105]`~`tri[119]`, **15칸(60바이트) 초과** |

### 스택 프레임에서 무엇을 덮었나

`build_pascal` 을 부르기 직전(`break 95`)과 직후(`next`)의 `main` 프레임이다.

```
(gdb) p &tri
$1 = (int (*)[105]) 0x7fffffffea20
(gdb) p (char*)$rbp - (char*)&tri
$2 = 432
(gdb) x/gx $rbp-8
0x7fffffffebc8:	0xb20bc160fc78e000
(gdb) x/2gx $rbp
0x7fffffffebd0:	0x00007fffffffec70	0x00007ffff7dcf1ca

(gdb) next
(gdb) x/15dw &tri[105]
0x7fffffffebc4:	1	14	91	364
0x7fffffffebd4:	1001	2002	3003	3432
0x7fffffffebe4:	3003	2002	1001	364
0x7fffffffebf4:	91	14	1
(gdb) x/gx $rbp-8
0x7fffffffebc8:	0x0000005b0000000e
(gdb) x/2gx $rbp
0x7fffffffebd0:	0x000003e90000016c	0x00000bbb000007d2
```

`main` 의 스택 프레임을 그리면 이렇다. 상자 왼쪽은 rbp 기준 오프셋(그 칸이 시작하는 주소), 오른쪽은 `break 95` 때 값 `->` `next` 뒤 값, 그리고 그 자리를 덮은 배열 원소다.

```
높은 주소
              +------------------------+
rbp+16 ~      |  caller (libc)         |   tri[112] ~ tri[119]
              +------------------------+
rbp+8         |  return address        |   0x00007ffff7dcf1ca -> 0x00000bbb000007d2   tri[110], tri[111]
              +------------------------+
rbp  ------>  |  saved rbp             |   0x00007fffffffec70 -> 0x000003e90000016c   tri[108], tri[109]
              +------------------------+
rbp-8         |  canary                |   0xb20bc160fc78e000 -> 0x0000005b0000000e   tri[106], tri[107]
              +------------------------+
rbp-12        |  (gap 4B)              |   -> 1   tri[105]   ← 여기부터 배열 밖
              +------------------------+
rbp-16        |  tri[104]              |
              |    ...    int tri[105] |   420B, 여기까지 정상
rbp-432       |  tri[0]                |   &tri = 0x7fffffffea20
              +------------------------+
rbp-436       |  int i                 |
              +------------------------+
              |  (padding 12B)         |
rbp-448 --->  +------------------------+   ← rsp   (sub $0x1c0,%rsp)
낮은 주소
```

**`x` 명령 읽는 법** — `x/[개수][모양][크기] 주소` : 그 주소부터 메모리를 정한 크기로 잘라 읽는다. 모양은 `x` 16진수 · `d` 10진수 · `i` 기계 명령, 크기는 `w` 4바이트 · `g` 8바이트.

| 명령 | 풀이 | 읽은 자리 |
| --- | --- | --- |
| `x/gx $rbp-8` | 8바이트 1칸을 16진수로 | 카나리 |
| `x/2gx $rbp` | 8바이트 2칸을 16진수로 | 저장된 rbp · 리턴 주소 |
| `x/15dw &tri[105]` | 4바이트 15칸을 10진수로 | `tri[105]`~`tri[119]` (rbp−12 ~ rbp+47) |

카나리 · rbp · 리턴 주소는 8바이트 값이라 `g`, 배열 원소는 `int`(4바이트)라 `w` 로 읽었다.

- 덮은 값은 14행 `1 14 91 364 1001 2002 3003 3432 …` 그대로다. 8바이트 한 칸에 `int` 두 개가 들어가 `0x0000005b0000000e` 처럼 보인다 — 낮은 주소의 14(`0x0e`)가 오른쪽 절반, 91(`0x5b`)이 왼쪽 절반이다(리틀 엔디언).
- **리턴 주소까지 실제로 덮였다** (`0x00000bbb000007d2`). 그래도 그 주소로 뛰지 않은 건 카나리 검사가 `ret` 보다 먼저라서다.

```
<+15>:  mov    %fs:0x28,%rax        ← 함수 시작: 비밀 값을 꺼내
<+24>:  mov    %rax,-0x8(%rbp)      ←   rbp-8 에 심는다 (카나리)
   ...
<+131>: mov    -0x8(%rbp),%rdx      ← 함수 끝: rbp-8 을 다시 읽어
<+135>: sub    %fs:0x28,%rdx        ←   원래 값과 빼 본다
<+144>: je     <main+151>           ←   같으면 leave · ret
<+146>: call   __stack_chk_fail     ←   다르면 여기 → abort → SIGABRT
<+151>: leave
<+152>: ret
```

## 4. 원인

| | 위치 |
| --- | --- |
| 죽은 곳 | `main` 끝(bug.c:112)의 카나리 검사 → `__stack_chk_fail` → `abort` |
| 진짜 원인 | `build_pascal` 의 행 루프 `for (int i = 0; i <= rows; i++)` (bug.c:66) |

**깨진 가정**: "`rows` 까지 돌면 딱 배열 크기만큼 채운다" — `rows`(14)는 행의 **개수**라서 행 번호는 0~13인데, `<=` 는 행 번호 14까지 간다.

**첫 단서**: 터미널의 `stack smashing detected` + bt에서 `__stack_chk_fail` 바로 아래 내 프레임이 **닫는 `}` 줄**이다. 그 함수의 지역 배열이 넘쳤다는 뜻이고, 넘쳐 쓴 곳은 그 함수이거나 배열 주소를 넘겨받은 함수다(여기선 `build_pascal`).

## 5. 해결

```c
static void build_pascal(int *tri, int rows)
{
    for (int i = 0; i < rows; i++)   /* <= → < : 유효 행은 0..rows-1 */
    {
        ...
    }
}
```

**왜 거기서 고쳤나**: 배열 크기 `SIZE` 는 0~13행을 담도록 정확히 잡혀 있다. 틀린 건 루프가 센 행의 수라서, 루프를 크기의 정의(0..rows-1)에 맞췄다.

검증: 일반 실행 exit 0 · ASan+UBSan 메모리 오류 없음.

## 6. 배운 점

- 스택은 높은 주소 → 낮은 주소로 자라지만, 배열 안에서는 인덱스가 커질수록 주소가 높아진다(`&tri[i] = tri + 4i`). 그래서 배열 끝을 넘으면 **먼저 자리 잡은 것들** — 카나리 → 저장된 rbp → 리턴 주소 — 을 차례로 덮는다
- **카나리는 원본과 사본을 비교한다.** 함수가 시작할 때 `%fs:0x28` 에 보관된 원본 값(무작위, 여기선 `0xb20bc160fc78e000`)을 rbp−8 에 사본으로 심고, 끝날 때 둘을 비교한다. 넘친 쓰기가 바꿀 수 있는 건 스택의 사본뿐이라, 다르면 들통난다. `0x28` 은 값이 아니라 원본을 보관하는 **자리**다
- **카나리는 배열과 저장된 rbp 사이에 있다.** 배열이 위로 넘치면 저장된 rbp · 리턴 주소보다 **먼저** 카나리를 밟는다. 그래서 `ret` 직전에 사본이 바뀌어 있으면 리턴 주소도 오염됐을 수 있다고 보고, `__stack_chk_fail` 이 `stack smashing detected` 를 찍고 `abort` 한다(SIGABRT). 02번은 실제로 리턴 주소까지 덮여 있었다
- 오버플로 자체는 조용하다. 카나리 검사는 그 카나리를 심은 함수(여기선 배열을 가진 `main`)가 `return` 하기 직전에만 한다. 넘쳐 쓴 건 `build_pascal` 이지만 걸린 건 `main` 이 끝날 때라, 출력이 다 나온 뒤에 죽었고 죽는 줄도 쓰기 줄이 아니라 `main` 의 `}` 다
- **카나리는 이어서 넘친 경우만 잡는다.** 비교하는 건 rbp−8 의 8바이트뿐이라, 카나리를 건너뛰고 리턴 주소 칸에만 쓰면(예: `tri[110]` 하나만) 검사를 통과한다. 02번은 `tri[105]` 부터 차례로 써서 카나리를 밟았기 때문에 걸렸다
- 인덱스 산술이 끼면 off-by-one이 안 보인다. **마지막으로 쓰는 인덱스**를 손으로 계산해 크기와 비교한다: `tri_index(14, 14) = 119 ≥ 105`
- 참고: CSAPP 3.10.3~3.10.4 (버퍼 오버플로 · 스택 보호). 책의 `%fs:40` 이 여기의 `%fs:0x28` 이다
