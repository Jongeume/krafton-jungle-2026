# 01. Use-After-Free — C로 만든 다형성이 무너지는 순간

> 위젯을 그리려던 함수가 **함수 주소 자리에 들어 있던 문자열 `"STATUS: "`** 로 점프하려다 죽었다.
> 원인은 자기 자신을 `free` 한 위젯, 그리고 그 위젯을 여전히 가리키던 배열 칸이다.

| 항목 | 내용 |
| --- | --- |
| 유형 | Use-After-Free (댕글링 포인터) |
| 신호 | `SIGSEGV` (종료 코드 139) |
| 환경 | x86-64 Linux · gcc `-g -O0 -fno-omit-frame-pointer` · gdb |
| 핵심 키워드 | 함수 포인터 테이블(vtable) · 구조체 오프셋 · glibc tcache 재사용 · 소유권 |

---

## 1. 증상

위젯 네 개(label, OK 버튼, dialog, cancel 버튼)를 가진 화면을 두 프레임 그리는 프로그램이다.
첫 프레임은 정상으로 그려지고, **두 번째 프레임의 렌더링에서 `SIGSEGV`** 로 죽는다.

---

## 2. 프로그램 구조 — C로 만든 "객체"

C에는 클래스가 없다. 이 프로그램은 **구조체 + 함수 포인터 테이블**로 객체와 다형성을 흉내 낸다.

```c
typedef struct VTable {
    void (*render)(Widget *w);
    void (*on_event)(Widget *w, int code);
} VTable;

typedef struct Widget {
    const VTable *vtbl;   /* 오프셋 0  — 종류별 함수 테이블 */
    int  id;              /* 오프셋 8 */
    int  closed;          /* 오프셋 12 */
    char label[24];       /* 오프셋 16 — sizeof(Widget) = 40 */
} Widget;

typedef struct Screen {
    Widget *items[8];        /* 위젯 "주소"만 모아 둔 포인터 배열 */
    int     count;
} Screen;
```

> 구조체 정의는 분석을 위해 멤버 구성만 요약한 것이다.

### 다형성: 부르는 쪽은 위젯 종류를 모른다

```c
for (int i = 0; i < s->count; i++) {
    Widget *w = s->items[i];
    w->vtbl->render(w);          /* 종류에 따라 다른 render 가 불린다 */
}
```

| 위젯 종류 | `render` | `on_event` |
| --- | --- | --- |
| label · button | 종류별 `*_render` | `widget_noop_event` (아무것도 안 함) |
| dialog | `dialog_render` | `dialog_on_event` ← **실제 동작은 여기뿐** |

- 같은 종류의 위젯은 **테이블 하나를 공유**한다 (`LABEL_VT`, `BUTTON_VT`, `DIALOG_VT`).
- `screen_render` 는 위젯이 버튼인지 dialog인지 몰라도 된다. 새 종류가 생겨도 이 루프는 그대로다.
- 자바의 인터페이스·동적 디스패치와 같은 생각이고, C++ 가상 함수가 실제로 이렇게 구현된다.

---

## 3. 실행 흐름

```
main
├─ screen_add ×4 → widget_new (malloc)      items[0] label · [1] ok · [2] dialog · [3] cancel
├─ [frame 1]
│  ├─ screen_render   → items[i]->vtbl->render(w)
│  ├─ screen_dispatch → items[i]->vtbl->on_event(w, code)
│  │    └─ dialog_on_event → widget_destroy(self) → free   ★ dialog 해제. items[2] 는 그대로
│  └─ app_build_status → malloc                            ★ 방금 해제된 조각을 돌려받음
│                        → "STATUS: dialog closed" 기록
├─ [frame 2]
│  └─ screen_render   → items[2]->vtbl == "STATUS: "  →  💥 SIGSEGV
└─ free(status) · free(items[i])
```

### 소유권

| 메모리 | 위치 | 할당 | 해제 | 가리키는 쪽 |
| --- | --- | --- | --- | --- |
| `Screen s` (`items` 배열 포함) | 스택 (main 지역) | 자동 | main 리턴 | — |
| 위젯 4개 | 힙 | `widget_new` | dialog만 `widget_destroy(self)`, 나머지는 main | `s.items[i]` |
| `VTable` 3개 | 전역 | 로드 시 | 해제 안 함 | 각 위젯의 `vtbl` |
| `status` 문자열 | 힙 | `app_build_status` | main | `status` |

**해제하는 쪽(`dialog_on_event`)과 가리키는 쪽(`s.items[2]`)이 다르다.** 여기서 문제가 시작된다.

---

## 4. gdb로 역추적 — 소스를 열기 전에

크래시가 난 뒤 **소스를 보지 않고** 어디까지 갈 수 있는지 먼저 확인했다.

```
(gdb) run
Program received signal SIGSEGV, Segmentation fault.

(gdb) bt
#0  screen_render (s=...)
#1  main () at bug.c:195            ← screen_render(&s) 두 번째 호출

(gdb) x/i $pc
=> mov (%rax),%rdx                  ← %rax 를 주소로 보고 8바이트를 읽으려 함

(gdb) p $_siginfo._sifields._sigfault.si_addr
$1 = (void *) 0x0                   ← NULL?

(gdb) p/x $rax
$2 = 0x203a535554415453             ← 주소처럼 안 생겼다

(gdb) p (char[8])$rax
$3 = "STATUS: "                     ← 포인터 자리에 문자열이 들어 있다
```

| 단계 | 알아낸 것 |
| --- | --- |
| 신호 | `SIGSEGV` — CPU가 잘못된 메모리 접근을 잡았다 |
| 죽은 명령 | `mov (%rax),%rdx` — `%rax` 가 가리키는 곳을 읽다가 죽었다 |
| 건드린 주소 | `si_addr = 0x0` 이지만 **NULL 역참조가 아니다** (아래 참고) |
| `%rax` 의 정체 | 바이트가 전부 `0x20`~`0x7e` → ASCII. `53 54 41 54 55 53 3a 20` = `"STATUS: "` |
| 원인 프레임 | `bt` 에는 `screen_render` ← `main` 만 보인다. **해제한 함수는 이미 리턴해서 안 보인다** |

### `si_addr = 0x0` 인데 NULL이 아닌 이유

x86-64는 주소의 위쪽 16비트가 규칙에 맞아야 하는 **정규(canonical) 주소**만 허용한다.
`0x203a535554415453` 은 그 규칙을 어기는 **비정규 주소**라서, 페이지 폴트가 아니라 일반 보호 예외(#GP)가 난다.
이때 커널은 어느 주소였는지 알려 주지 않으므로 `si_addr` 가 `0x0` 으로 찍힌다.
**그래서 `si_addr` 보다 `x/i $pc` 가 지목한 레지스터를 먼저 봐야 한다.**

### 재사용 확인 — 포인터의 "값"을 비교한다

```
(gdb) p s->items                       # frame 0 (screen_render)
$14 = {0x5555555592a0, 0x5555555592d0, 0x555555559300, 0x555555559330, 0x0, 0x0, 0x0, 0x0}

(gdb) p s->items[2]                    # 해제된 dialog를 여전히 가리키는 칸
$13 = (Widget *) 0x555555559300

(gdb) p status                         # frame 1 (main) — 새로 malloc 된 문자열
$15 = 0x555555559300 "STATUS: dialog closed"
```

**두 값이 `0x555555559300` 으로 같다.** 해제된 dialog 자리를 문자열이 그대로 재사용했다.

위젯 주소들은 정확히 **`0x30`(48바이트) 간격**이다.

```
0x5555555592a0 → 0x5555555592d0 → 0x555555559300 → 0x555555559330
          +0x30            +0x30            +0x30
```

`Widget` 40바이트 + glibc가 조각 앞에 붙이는 크기 정보 8바이트를 16의 배수로 맞추면 48바이트다.
`status` 요청도 같은 48바이트 칸에 들어가는 크기였기 때문에, 방금 해제된 `…9300` 이 그대로 돌아왔다.

> ⚠️ `p &s.items[2]`, `p &status` 처럼 `&` 를 붙이면 **포인터 변수가 스택 어디에 있는지**(`0x7fff…`)가 나온다.
> 재사용을 확인하려면 `&` 없이 **담긴 값**(`0x5555…`, 힙)을 비교해야 한다.

---

## 5. 원인

1. `dialog_on_event` 가 `widget_destroy(self)` 로 **자기 자신을 해제**한다.
2. 하지만 `s.items[2]` 는 여전히 해제된 주소를 가리킨다 (**댕글링 포인터**).
3. `app_build_status` 의 `malloc` 이 **방금 해제된 그 조각을 돌려받아** `"STATUS: dialog closed"` 를 쓴다.
4. 두 번째 `screen_render` 가 `items[2]->vtbl` 을 읽는데, 그 자리(오프셋 0)에는 문자열의 첫 8바이트 `"STATUS: "` 가 들어 있다.
5. 그걸 함수 테이블 주소로 믿고 `vtbl->render` 를 읽으려다(`mov (%rax),%rdx`) 죽는다.

### 왜 하필 그 조각이 재사용됐나 — 우연이 아니다

glibc는 해제된 조각을 **크기별 목록(tcache)** 에 넣고, 같은 크기의 요청이 오면 **가장 최근에 해제된 조각부터** 돌려준다(LIFO).
`Widget`(40바이트 → 48바이트 조각)과 `status` 요청이 같은 크기 칸에 들어가서, 방금 해제된 dialog 자리(`0x555555559300`)가 그대로 재사용됐다. 같은 크기라면 **거의 확정적으로** 일어나는 일이다.

### 왜 하필 `vtbl` 이 깨졌나 — 구조체 오프셋

`vtbl` 이 `Widget` 의 **오프셋 0** 에 있기 때문이다. 재사용한 쪽이 조각의 맨 앞부터 쓰면, 제일 먼저 덮이는 게 함수 테이블 포인터다.
다형성의 핵심인 `vtbl` 이 UAF에서 가장 먼저 무너지는 자리이기도 하다.

---

## 6. 해결 — 해제는 "칸을 가진 쪽"이

```c
/* dialog_on_event: 자기를 해제하지 않고 상태만 바꾼다 */
if (code == 1)
    self->closed = 1;

/* screen_dispatch: 위젯을 가진 화면이 해제하고, 칸도 비운다 */
if (w->closed) {
    widget_destroy(w);
    s->items[i] = NULL;
}

/* screen_render: 비워진 칸은 건너뛴다 */
if (w != NULL)
    w->vtbl->render(w);
```

- **위젯은 자기가 화면의 몇 번 칸에 꽂혀 있는지 모른다.** `self` 로는 자기 내용만 바꿀 수 있고, 자기를 가리키는 칸은 못 비운다. 그러니 해제할 자격도 없다.
- 자기를 해제해 버리면 `closed` 값조차 해제된 메모리가 되어, "닫혔다"는 신호로도 쓸 수 없다.
- 그래서 **위젯은 상태만 바꾸고(`closed = 1`), 칸을 가진 `screen` 이 핸들러로서 해제와 칸 정리를 함께** 한다.
- 마지막 정리 루프의 `free(s.items[i])` 는 비워진 칸에서 `free(NULL)` 이 되는데, C 표준상 `free(NULL)` 은 아무것도 하지 않으므로 이중 해제도 없다.

---

## 7. 배운 점

- **`free` 는 포인터를 지우지 않는다.** 비웠어야 할 건 메모리가 아니라 그 메모리를 가리키던 칸이다.
- 해제된 메모리는 **재사용되기 전까지는 멀쩡해 보인다.** 그래서 UAF는 들쭉날쭉하고, 크래시 지점과 원인 지점이 멀다.
- 포인터처럼 안 생긴 값을 만나면 `p (char[8])$reg` 로 문자로 읽어 본다.
- **책임과 역할을 먼저 정해야 한다.** 누가 만들고, 누가 가리키고, 누가 치우는지를 설계에서 정하지 않으면 이런 버그가 생긴다.

### 자바와 비교하면

| C로 직접 한 것 | 자바에서는 |
| --- | --- |
| 구조체 맨 앞의 `vtbl` + `w->vtbl->render(w)` | `w.render()` — JVM이 알맞은 메서드를 찾아 준다 |
| 해제된 위젯을 가리키는 칸 → UAF | GC가 참조가 남아 있으면 해제하지 않는다 |
| gdb로 레지스터를 읽어 원인 추적 | 예외와 스택 트레이스 |

자바가 `w.render()` 한 줄로 해 주던 걸 C로 해 보니, **그 한 줄이 무엇을 막아 주고 있었는지** 보였다.
다만 "누가 치우나"는 자바에서도 남는다. 다 쓴 객체를 리스트에서 빼지 않으면 GC가 치우지 못해 메모리 누수가 된다.

---

## gdb 명령 요약

| 명령 | 용도 |
| --- | --- |
| `bt` | 호출 스택 — 어디서 죽었나 |
| `x/i $pc` | 죽은 어셈블리 한 줄 — 어느 레지스터를 썼나 |
| `p/x $rax` | 그 레지스터의 값 |
| `p (char[8])$rax` | 레지스터 8바이트를 문자로 읽기 |
| `p $_siginfo._sifields._sigfault.si_addr` | 건드린 주소 (비정규 주소면 `0x0`) |
| `frame 1` · `info locals` | 내 코드 프레임으로 올라가 지역 변수 보기 |
| `p s->items[2]` · `p status` | 두 포인터의 **값** 비교 — 재사용 확인 (둘 다 `0x555555559300`) |
