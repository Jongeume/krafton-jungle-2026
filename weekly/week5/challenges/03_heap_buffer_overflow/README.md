# 03. Heap Buffer Overflow — 용량은 늘었는데 버퍼는 그대로였다

> 정수 동적 배열이 두 번째로 자라려는 순간 `realloc(): invalid next size` 로 죽었다.
> 원인은 `realloc` 에 새 용량이 아니라 옛 용량을 넘긴 것 — 장부(`cap`)만 2배가 되고 실제 버퍼는 그대로라, push 가 버퍼 밖 힙을 덮었다.

| 항목 | 내용 |
| --- | --- |
| 유형 | Heap Buffer Overflow |
| 신호 | `SIGABRT` (`realloc(): invalid next size`) |
| 환경 | x86-64 Linux · gcc `-g -O0 -fno-omit-frame-pointer` · gdb |

## 1. 증상

정수 동적 배열 `IntList`(`data` · `len` · `cap`)에 0~1,999,999 를 100으로 나눈 나머지를 차례로 넣고, 길이 · 용량 · 합을 출력하는 프로그램이다. 용량이 차면 `list_ensure` 가 용량을 2배로 늘려 `realloc` 한다.
아무것도 출력하지 못하고 `realloc(): invalid next size` 와 함께 `SIGABRT` 로 죽는다.

## 2. gdb 역추적 — 소스를 열기 전에

```
(gdb) run
realloc(): invalid next size

Program received signal SIGABRT, Aborted.

(gdb) bt
#0  0x00007ffff7e43c0c in pthread_kill () from /lib/x86_64-linux-gnu/libc.so.6
#1  0x00007ffff7dea27e in raise () from /lib/x86_64-linux-gnu/libc.so.6
#2  0x00007ffff7dcd8ff in abort () from /lib/x86_64-linux-gnu/libc.so.6
#3  0x00007ffff7dce7b6 in ?? () from /lib/x86_64-linux-gnu/libc.so.6
#4  0x00007ffff7e4e0d5 in ?? () from /lib/x86_64-linux-gnu/libc.so.6
#5  0x00007ffff7e5221c in ?? () from /lib/x86_64-linux-gnu/libc.so.6
#6  0x00007ffff7e533f5 in realloc () from /lib/x86_64-linux-gnu/libc.so.6
#7  0x00005555555552f2 in list_ensure (l=0x7fffffffebb0, need=17) at bug.c:68
#8  0x0000555555555384 in list_push (l=0x7fffffffebb0, x=16) at bug.c:76
#9  0x00005555555554b0 in main () at bug.c:98

(gdb) x/i $pc
=> 0x7ffff7e43c0c <pthread_kill+284>:   mov    %eax,%r14d
```

| 칸 | 알아낸 것 |
| --- | --- |
| 신호 | `SIGABRT` — glibc 가 스스로 `abort()` 를 불렀다 |
| 죽은 명령 | `realloc` 안에서 `abort` + 터미널 `realloc(): invalid next size`. `x/i $pc` 는 libc `pthread_kill` 안이라 단서가 아니다 |
| 건드린 주소 · 구역 | 해당 없음 (SIGABRT). 메시지로 보아 **힙 청크**(chunk)의 헤더가 깨졌다 |
| 원인 프레임 | #7 `list_ensure` (bug.c:68) 의 `realloc` 호출. `need=17` — 16칸이 차서 두 번째로 자라려던 순간 |
| 원인 한 문장 | `realloc` 에 옛 용량을 넘겨 버퍼가 안 자랐고, 그 뒤 push 가 버퍼 밖 힙 정보를 덮어 다음 `realloc` 검사에 걸렸다 |

## 3. 코드 흐름

**② 경계 버그** — 두 숫자 비교

| 크기가 정해지는 곳 | 인덱스가 커지는 곳 | 어긋나는 순간 |
| --- | --- | --- |
| `realloc(l->data, l->cap * sizeof(int))` — **옛 용량**만큼만 확보 (bug.c:68) | `l->cap = newcap` 으로 장부는 2배 → push 가 `data[len]` 을 `cap` 까지 쓴다 | 첫 `list_ensure`(8 → 16): `realloc(p, 32)` 는 같은 크기라 버퍼는 8칸 그대로인데 `cap = 16` → `data[8]`~`data[15]` 가 버퍼 밖 |

### 힙에서 무엇을 덮었나

두 번의 `list_ensure` 에서 `realloc` 을 부르기 직전의 힙이다.

```
(gdb) break 68 if need == 9          ← 첫 번째: 8칸이 찼다
(gdb) p l->data
$3 = (int *) 0x5555555592a0
(gdb) x/gx (char*)l->data - 8
0x555555559298:	0x0000000000000031
(gdb) x/2gx (char*)l->data + 32
0x5555555592c0:	0x0000000000000000	0x0000000000020d41
(gdb) next
(gdb) p p
$4 = (int *) 0x5555555592a0          ← realloc(p, 32): 같은 크기라 같은 주소

(gdb) break 68 if need == 17         ← 두 번째: 16칸을 썼다
(gdb) x/2gx (char*)l->data + 32
0x5555555592c0:	0x0000000900000008	0x0000000b0000000a
(gdb) x/16dw l->data
0x5555555592a0:	0	1	2	3
0x5555555592b0:	4	5	6	7
0x5555555592c0:	8	9	10	11
0x5555555592d0:	12	13	14	15
(gdb) continue
realloc(): invalid next size
```

glibc 는 `malloc` 이 준 주소 바로 앞 8바이트에 그 청크의 **헤더(`size` 필드)** 를 둔다. 청크는 `malloc` 이 관리하는 힙 메모리 한 덩어리다(CSAPP 의 블록). `malloc(32)` 은 48바이트(`0x30`) 청크를 받았고, 그 바로 뒤가 아직 안 쓴 힙 끝(**top chunk**)이다.

```
낮은 주소
0x...298       | size = 0x31               |   이 청크의 헤더(size): 48B (0x30) + 1
               +---------------------------+
0x...2a0  -->  | data[0] ~ data[7]         |   l->data. malloc(32) 로 받은 32B = 8칸
               +===========================+   여기부터 다음 청크 (top chunk)
0x...2c0       | prev_size                 |   0x0     -> 0x0000000900000008   data[8], data[9]
0x...2c8       | size = 0x20d41            |   0x20d41 -> 0x0000000b0000000a   data[10], data[11]
               +---------------------------+
0x...2d0       | (free space)              |   data[12] ~ data[15]
높은 주소
```

- 버퍼 밖에 쓴 `data[10]`, `data[11]` 이 **다음 청크의 헤더(`size`)** 를 `0x0000000b0000000a` 로 덮었다.
- 두 번째 `realloc(p, 64)` 는 늘리기 전에 바로 다음 청크의 크기를 확인하는데, 그 값이 망가져 있어서 `invalid next size` 로 멈췄다.
- 힙 안에서 넘친 쓰기는 그 순간엔 아무 일도 없다(같은 힙 페이지라 SIGSEGV 도 안 난다). glibc 가 **다음에 그 청크를 다룰 때** 드러난다.

## 4. 원인

| | 위치 |
| --- | --- |
| 죽은 곳 | 두 번째 `list_ensure` 의 `realloc` (bug.c:68, `need=17`) → glibc 힙 검사 → `abort` |
| 진짜 원인 | 같은 줄의 인자 `l->cap * sizeof(int)` — 새 용량 `newcap` 이 아니라 옛 용량 (첫 `list_ensure` 부터 틀렸다) |

**깨진 가정**: "`cap` 을 늘렸으니 버퍼도 늘었다" — `cap` 은 장부일 뿐이고, 실제 크기는 `realloc` 에 넘긴 값이 정한다. 첫 `list_ensure` 는 `realloc(p, 32)` 로 아무것도 안 늘렸는데 `cap` 만 16 이 됐다.

**첫 단서**: bt 의 `#6 realloc` + 터미널 `realloc(): invalid next size` → 힙 청크의 헤더가 깨졌다 = 누군가 힙 버퍼 끝을 넘어 썼다. `realloc` 은 **발견한** 곳이고, **쓴** 곳은 그 전의 `list_push` 다.

## 5. 해결

```c
int *p = realloc(l->data, newcap * sizeof(int));   /* l->cap → newcap */
```

**왜 거기서 고쳤나**: 버퍼 크기를 정하는 건 `realloc` 의 인자다. `l->cap = newcap` 은 `realloc` 이 성공한 뒤에 하는 지금 순서가 맞으니, 인자만 새 용량으로 맞췄다. 이제 장부(`cap`)와 실제 크기가 항상 같다.

```
len=2000000 cap=2097152 sum=99000000
```

검증: 일반 실행 exit 0 · ASan+UBSan 메모리 오류 없음 · 누수 없음 · 미초기화 검사 통과.

## 6. 배운 점

- 힙 오버플로는 쓰는 순간 조용하다. glibc 가 **다음** `malloc` · `realloc` · `free` 에서 청크 헤더(`size`)를 검사할 때 드러난다. `realloc(): invalid next size` 는 "이 청크 뒤를 누가 넘어 썼다" 는 뜻이다
- 02 스택은 카나리가 **return 할 때**, 03 힙은 glibc 가 **다음 realloc 에서** 잡았다. 넘친 구역마다 알아채는 쪽과 때가 다르다
- 용량 필드(`cap`)와 실제 확보량(`realloc` 인자)을 따로 계산하면 어긋난다. 둘 다 같은 값(`newcap`)에서 나오게 한다

### realloc 은 늘려도 주소가 그대로일 수 있다

처음엔 "크기가 같으면 같은 주소, 커지면 옛 자리를 free 하고 다른 주소에 새로 잡는다" 고 알고 있었는데, 늘렸는데도 주소가 그대로였다. 고친 코드의 `list_ensure` 에 주소를 찍는 줄 하나를 넣어 돌려 보면 이렇다 (gdb 밖에서 돌려 주소가 `0x5555…` 가 아니다).

```
cap        8 ->       16  (       64 B)  0x56104bf3f2a0 -> 0x56104bf3f2a0  same
cap       16 ->       32  (      128 B)  0x56104bf3f2a0 -> 0x56104bf3f2a0  same
  ...                                    (여기까지 계속 same)
cap    16384 ->    32768  (   131072 B)  0x56104bf3f2a0 -> 0x56104bf3f2a0  same
cap    32768 ->    65536  (   262144 B)  0x56104bf3f2a0 -> 0x7a16a95ac010  MOVED
cap    65536 ->   131072  (   524288 B)  0x7a16a95ac010 -> 0x7a16a952b010  MOVED
  ...                                    (여기부터 계속 MOVED)
cap  1048576 ->  2097152  (  8388608 B)  0x7a16a8e28010 -> 0x7a16a8627010  MOVED
```

- **128KB 까지는 제자리**: 이 버퍼는 힙의 마지막 청크라 바로 뒤가 top chunk(처음 약 131KB 여유, `0x20d41` → 134,464바이트)다. `realloc` 은 뒤가 비어 있으면 **그 자리에서 늘린다**
- **256KB 에서 처음 옮겨졌다**: 힙 끝의 여유로는 더 못 늘려 새 자리가 필요했고, 새 자리는 힙(`0x56…`) 밖의 `0x7a…` 영역이었다. glibc 는 큰 요청(기본 128KB 이상)을 힙 대신 `mmap` 으로 받는다 (`mallopt(3)` 의 `M_MMAP_THRESHOLD`). 그 뒤로는 이미 mmap 청크라 아래의 `mremap` 길로 갔다

#### realloc 은 언제 옮기나 (glibc 기준)

C 표준은 "내용은 유지한다 · 주소는 바뀔 수 있다" 까지만 약속한다. 언제 옮기는지는 구현이 정한다. glibc 는 이렇게 고른다.

```
realloc(p, 새 크기)
├─ 줄이거나 같다 → 제자리 (남는 뒷부분이 충분히 크면 잘라서 free)
└─ 늘린다
   ├─ p 가 원래 mmap 으로 받은 큰 청크 → mremap (커널이 매핑을 늘리거나 다른 주소로 옮김)
   ├─ 바로 뒤가 top chunk 이고 여유가 충분 → 제자리 (top 에서 떼어 붙임)
   ├─ 바로 뒤가 빈 청크이고 합치면 충분 → 제자리 (뒤 청크를 흡수)
   └─ 그 외 (뒤가 사용 중 · 합쳐도 모자람) → 새 자리 malloc → 내용 복사 → 옛 자리 free
```

**옮기는 건 바로 뒤에 붙여 쓸 빈 공간이 없을 때뿐이다.** 청크는 연속된 한 덩어리여야 하니, 뒤가 막히면 더 큰 빈 곳으로 옮길 수밖에 없다.

| 경우 | 주소 | 복사 | 옛 자리 free |
| --- | --- | --- | --- |
| 줄이기 · 같은 크기 | 그대로 | 없음 | 없음 (남는 뒷부분만 떼어 free) |
| 뒤가 top · 빈 청크라 제자리 확장 | 그대로 | 없음 | 없음 |
| 이동 (새 자리 malloc) | 바뀜 | 옛 크기만큼 `memcpy` | 있음 (복사 **뒤에**) |
| mmap 청크 (`mremap`) | 그대로 또는 바뀜 | 바이트 복사 대신 커널이 페이지 매핑을 옮김 | 없음 |

**제자리일 때는 복사도 free 도 없다.** 데이터는 이미 그 자리에 있으니 옮길 게 없고, 청크 헤더의 `size` 만 고쳐 쓴다.

```
before   [ size 0x30 | data[0..7]            ][ top (free) ........................ ]
                         realloc(p, 64): next is top -> grow in place
after    [ size 0x50 | data[0..7] + 8 more   ][ top (smaller) ..................... ]
```

- 처음 알던 "커지면 free 하고 다른 주소로" 는 **옮기는 경우에만** 맞다. 순서도 free 가 먼저가 아니라 **새 자리 확보 → 복사 → 옛 자리 free** 다 (먼저 free 하면 옮길 내용이 사라진다)
- **tcache 함정**: tcache 에 들어간 작은 청크는 free 됐어도 glibc 가 "사용 중" 으로 표시해 둔다. 그래서 바로 뒤가 방금 free 한 작은 청크여도 합치지 못하고 옮겨질 수 있다
- **용량을 2배씩 늘리는 이유**: 제자리 확장은 헤더의 `size` 만 고치니 싸고, 이동은 옛 크기만큼 복사하니 비싸다. 2배씩 늘리면 매번 옮겨도 옮긴 양을 다 더해 최종 용량보다 작다(8 + 16 + … + N/2 < N). push 한 번당 복사는 평균 1칸이 안 된다
- 그래서 `realloc` 뒤에는 **돌려받은 주소를 반드시 새로 써야** 한다 (`l->data = p`). 같을지 다를지는 그때 힙 상황이 정한다
