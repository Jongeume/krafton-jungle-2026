# 04. Double Free — 두 목록이 같은 레코드를 각자 free 했다

> 직원 레코드를 ID 순 · 이름 순 두 배열로 관리하는 프로그램이, 정리하다가 `free(): double free detected in tcache 2` 로 죽었다.
> 원인은 두 배열이 **같은 `Rec` 를 가리키는데**(별칭) 정리 함수가 두 배열을 각각 돌며 free 한 것 — `by_id` 로 이미 해제한 레코드를 `by_name` 으로 다시 해제했다.

| 항목 | 내용 |
| --- | --- |
| 유형 | Double Free |
| 신호 | `SIGABRT` (`free(): double free detected in tcache 2`) |
| 환경 | x86-64 Linux · gcc `-g -O0 -fno-omit-frame-pointer` · gdb |

## 1. 증상

직원 레코드 `Rec`(id, name) 4개를 힙에 만들어, 넣은 순서 그대로인 `by_id` 와 이름 순으로 정렬한 `by_name` 두 배열에 함께 넣는다. 두 배열을 출력하고 id 로 한 명을 찾은 뒤, 끝에서 `directory_free` 로 정리한다.
정리하는 도중 `SIGABRT` 로 죽고, 마지막 `done` 은 찍히지 않는다.

## 2. gdb 역추적 — 소스를 열기 전에

```
(gdb) run
free(): double free detected in tcache 2

Program received signal SIGABRT, Aborted.

(gdb) bt
#0  0x00007ffff7e43c0c in pthread_kill () from /lib/x86_64-linux-gnu/libc.so.6
#1  0x00007ffff7dea27e in raise () from /lib/x86_64-linux-gnu/libc.so.6
#2  0x00007ffff7dcd8ff in abort () from /lib/x86_64-linux-gnu/libc.so.6
#3  0x00007ffff7dce7b6 in ?? () from /lib/x86_64-linux-gnu/libc.so.6
#4  0x00007ffff7e4e0d5 in ?? () from /lib/x86_64-linux-gnu/libc.so.6
#5  0x00007ffff7e5063f in ?? () from /lib/x86_64-linux-gnu/libc.so.6
#6  0x00007ffff7e52eae in free () from /lib/x86_64-linux-gnu/libc.so.6
#7  0x0000555555555668 in directory_free (d=0x7fffffffeac0) at bug.c:108
#8  0x00005555555557a1 in main () at bug.c:127

(gdb) x/i $pc
=> 0x7ffff7e43c0c <pthread_kill+284>:   mov    %eax,%r14d
```

| 칸 | 알아낸 것 |
| --- | --- |
| 신호 | `SIGABRT` — glibc 가 스스로 `abort()` 를 불렀다 |
| 죽은 명령 | `free` 안에서 `abort` + 터미널 `free(): double free detected in tcache 2`. `x/i $pc` 는 libc `pthread_kill` 안이라 단서가 아니다 |
| 건드린 주소 · 구역 | 해당 없음 (SIGABRT). 메시지로 보아 **힙** — 이미 해제된 청크(chunk)를 또 해제 |
| 원인 프레임 | #7 `directory_free` (bug.c:108) — 둘째 루프의 `free(d->by_name[i])`, 이때 `i = 0` |
| 원인 한 문장 | `by_id` 루프에서 이미 free 한 `Rec` 를 `by_name` 루프에서 다시 free 했다 |

## 3. 코드 흐름

**① 수명 버그** — 메모리 하나의 생애 (alice 의 `Rec`, `0x5555555592e0`)

| 단계 | 어디서 | 무슨 일 |
| --- | --- | --- |
| 태어남 | `rec_new` (bug.c:59) | `malloc` 으로 alice 의 `Rec` 를 만든다 |
| 복사해 감 | `directory_add` (bug.c:70~71) | `by_id[1]` 과 `by_name[1]` 두 칸에 **같은 주소**를 넣는다. 이름 순 정렬 뒤엔 `by_name[0]` |
| 죽음 | `directory_free` 첫 루프 (bug.c:105) | `free(d->by_id[1])` |
| 죽은 뒤 씀 | `directory_free` 둘째 루프 (bug.c:108) | `free(d->by_name[0])` — 같은 주소를 또 free → abort |

### free 뒤에도 포인터는 그대로다

첫 루프 전과, 첫 루프가 `by_id` 로 4개를 모두 free 한 뒤의 두 배열이다.

```
(gdb) break 104
(gdb) run
(gdb) p d->by_id
$1 = {0x5555555592a0, 0x5555555592e0, 0x555555559320, 0x555555559360, 0x0 <repeats 12 times>}
(gdb) p d->by_name
$2 = {0x5555555592e0, 0x555555559360, 0x5555555592a0, 0x555555559320, 0x0 <repeats 12 times>}
(gdb) p d->by_name[0] == d->by_id[1]
$3 = 1
(gdb) x/2gx d->by_id[1]
0x5555555592e0:	0x0000000000000001	0x0000555555559300        ← id = 1, name 포인터

(gdb) delete 1
(gdb) break 107                                              ← 첫 루프가 끝난 자리
(gdb) continue
(gdb) p d->by_id
$4 = {0x5555555592a0, 0x5555555592e0, 0x555555559320, 0x555555559360, 0x0 <repeats 12 times>}
(gdb) p d->by_name
$5 = {0x5555555592e0, 0x555555559360, 0x5555555592a0, 0x555555559320, 0x0 <repeats 12 times>}
(gdb) x/2gx d->by_id[0]
0x5555555592a0:	0x000055500000c799	0x97996b58ccd2f845
(gdb) x/2gx d->by_id[1]
0x5555555592e0:	0x000055500000c659	0x97996b58ccd2f845
(gdb) x/2gx d->by_id[2]
0x555555559320:	0x000055500000c619	0x97996b58ccd2f845
(gdb) continue
free(): double free detected in tcache 2
```

- 4개를 모두 free 한 뒤에도 `by_id` · `by_name` 의 값은 **하나도 안 바뀌었다.** NULL 이 된 칸이 없다
- 대신 free 된 `Rec` 16바이트의 내용이 바뀌었다. glibc 가 해제된 청크를 **tcache**(크기별 free 청크 리스트 = bin 들의 묶음)에 넣으면서 앞 8바이트엔 같은 bin 의 다음 청크 주소를 섞어 저장한 값(`next`)을, 뒤 8바이트엔 **"tcache 에 들어 있음" 표시 값(`key`)** 을 쓴다. 세 청크의 뒤 8바이트가 모두 같은 값인 이유다 (`key` 값은 실행마다 달랐다)
- 둘째 루프가 `free(0x5555555592e0)` 를 부르면, glibc 는 그 청크의 뒤 8바이트(`key`)가 표시 값과 같은 걸 보고 그 bin 을 훑는다. 이미 들어 있으니 `double free detected in tcache 2` 로 멈춘다

## 4. 원인

| | 위치 |
| --- | --- |
| 죽은 곳 | `directory_free` 둘째 루프 `free(d->by_name[i])` (bug.c:108, `i = 0`) → glibc tcache 이중 해제 검사 → `abort` |
| 진짜 원인 | 같은 함수의 두 루프가 각자 free — `by_id` 와 `by_name` 은 같은 `Rec` 를 가리키는 별칭(bug.c:70~71)인데 둘 다 주인처럼 해제했다 |

**깨진 가정**: "배열이 둘이면 객체도 둘" — 두 배열은 같은 `Rec` 4개를 가리키는 서로 다른 목록일 뿐이다. 이름 순 정렬도 포인터 순서만 바꿨다.

**첫 단서**: 터미널의 `double free detected` + bt 의 #7 이 `free` 를 부른 내 줄이다. 그 줄의 포인터가 앞에서 free 한 다른 포인터와 같은 주소인지 `p a == b` 로 비교한다 (`p d->by_name[0] == d->by_id[1]` → `1`).

## 5. 해결

```c
static void directory_free(Directory *d) {
    for (int i = 0; i < d->count; i++) {      /* 소유 인덱스 by_id 에서만 해제 */
        free(d->by_id[i]->name);
        free(d->by_id[i]);
    }
    /* by_name 을 도는 둘째 루프는 지웠다 — 같은 Rec 를 빌려 보는 목록이다 */
    d->count = 0;
}
```

**왜 거기서 고쳤나**: `Rec` 는 `directory_add` 에서 한 번 만들어지니 해제도 한 번이어야 한다. 모든 `Rec` 를 한 번씩 가진 `by_id` 를 **주인**(소유 인덱스)으로 정해 여기서만 해제하고, 정렬만 다른 `by_name` 은 빌려 보는 목록이라 해제하지 않는다. `name` 은 `Rec` 가 가진 것이라 `Rec` 와 함께 `by_id` 루프에서 해제한다.

```
by id:  3:carol 1:alice 4:dave 2:bob 
by name: alice(1) bob(2) carol(3) dave(4)
lookup id=2 -> bob
done
```

검증: 일반 실행 exit 0 · ASan+UBSan 메모리 오류 없음 · 누수 없음 · 미초기화 검사 통과.

## 6. 배운 점

- **`free` 는 포인터를 NULL 로 만들지 않는다.** C 는 인자를 값으로 넘기니 `free(p)` 는 주소의 사본만 받고, 호출한 쪽의 변수를 바꿀 수 없다. `free` 뒤 `p` 는 해제된 곳을 가리키는 댕글링 포인터로 남는다 (위 gdb 에서 4개를 free 한 뒤에도 배열 값이 그대로였다)
- **`free(p); p = NULL;` 도 별칭은 못 막는다.** `free(NULL)` 이 아무 일도 안 하는 건 맞다(C 표준). 하지만 NULL 로 만드는 건 그 변수 하나(by_id 안의 주소)뿐이고, 같은 주소를 든 다른 변수(by_name 안의 주소)는 그대로 남는다

```
by_id   = { carol, alice, dave, bob }     ← 넣은 순서
by_name = { alice, bob, carol, dave }     ← 이름 순 정렬 뒤

free(by_id[1]); by_id[1] = NULL;          // alice 를 해제하고 by_id 칸만 비움
by_name[0]  ->  0x5555555592e0            // 여전히 alice 의 주소
```

- **정렬 뒤엔 같은 번호가 같은 레코드가 아니다.** `by_id[1]`(alice) 은 `by_name[0]` 이다. 그래서 같은 `i` 로 두 배열을 함께 비울 수 없다. 주소를 비교하며(`by_name[j] == r`) 뒤지면 찾을 수는 있지만, 레코드 하나를 지울 때마다 다른 목록을 다 확인해야 하고 목록이 늘면 챙길 곳도 는다
- **그래서 별칭을 NULL 로 하나하나 지우지 않고, 해제하는 주인을 한 곳으로 정했다** (`by_id` = 소유, `by_name` = 빌려 봄). NULL 은 해제한 뒤에도 **그 칸을 다시 읽는 코드가 있을 때** 의미가 있다

| | 해제 뒤 그 칸을 다시 읽나 | 어떻게 막았나 |
| --- | --- | --- |
| 01 | 읽는다 — frame 2 의 `render` 와 마지막 `free` 루프가 `items[2]` 를 다시 본다 | 칸 번호 `i` 를 아는 화면이 해제하고, 그 칸을 NULL 로 비운다 |
| 04 | 안 읽는다 — `directory_free` 뒤로 아무도 배열을 안 본다 | 해제하는 주인을 `by_id` 한 곳으로. `by_name` 은 free 하지 않는다 |

NULL 은 **그 칸을 가진 쪽이 자기 칸을 비우는 도구**이고, 별칭 문제는 **누가 주인인가**를 정해서 푼다. 01 과 04 가 같은 질문(누가 치우나)에 다른 답을 낸 이유다.

- glibc 는 tcache 에 넣은 청크에 `key` 를 적어 두고, `key` 가 있는 청크가 다시 `free` 로 오면 그 bin 을 훑어 이중 해제를 잡는다. 그래서 이 버그는 조용히 넘어가지 않고 바로 abort 로 드러났다
