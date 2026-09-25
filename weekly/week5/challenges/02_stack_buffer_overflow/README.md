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

| 자리 (rbp 기준) | 원래 들어 있던 것 | `build_pascal` 뒤 |
| --- | --- | --- |
| rbp−432 ~ rbp−13 | `tri[0]`~`tri[104]` (420바이트) | 정상 |
| rbp−12 | 남는 4바이트 | `tri[105]` = 1 |
| rbp−8 | **카나리** `0xb20bc160fc78e000` | `tri[106]`, `tri[107]` = 14, 91 |
| rbp | 저장된 rbp | `tri[108]`, `tri[109]` = 364, 1001 |
| rbp+8 | **리턴 주소** `0x7ffff7dcf1ca` | `tri[110]`, `tri[111]` = 2002, 3003 |
| rbp+16 ~ | `main` 을 부른 쪽의 영역 | `tri[112]`~`tri[119]` |

- 덮은 값은 14행 `1 14 91 364 1001 2002 3003 3432 …` 그대로다. `0x5b` = 91, `0x0e` = 14.
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
- 오버플로 자체는 조용하다. 카나리 검사는 함수가 `return` 하기 직전에만 하니, 죽는 줄이 쓰기 줄이 아니라 `}` 다
- 인덱스 산술이 끼면 off-by-one이 안 보인다. **마지막으로 쓰는 인덱스**를 손으로 계산해 크기와 비교한다: `tri_index(14, 14) = 119 ≥ 105`
- 참고: CSAPP 3.10.3~3.10.4 (버퍼 오버플로 · 스택 보호). 책의 `%fs:40` 이 여기의 `%fs:0x28` 이다
