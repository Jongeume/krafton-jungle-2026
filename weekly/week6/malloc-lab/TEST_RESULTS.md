# malloc-lab 테스트 결과

- 기준 코드 : `2c694ea` 명시적 가용 리스트 (LIFO) + mm_realloc 제자리 최적화
- 환경 : Dev Container (Ubuntu 24.04 · gcc 13.3 · `-m32`)
- 실행 : `make clean && make && ./mdriver -V` (짧은 trace 는 `./mdriver -V -f short1-bal.rep`)

## 짧은 trace

| 이슈 | trace | valid | util |
| --- | --- | --- | --- |
| #114 | short1-bal | yes | 66% |
| #115 | short2-bal | yes | 89% |

## 기본 trace 11개

| 이슈 | trace | valid | util | Kops |
| --- | --- | --- | --- | --- |
| #116 | amptjp-bal | yes | 89% | 20,541 |
| #117 | cccp-bal | yes | 92% | 48,411 |
| #118 | cp-decl-bal | yes | 94% | 20,952 |
| #119 | expr-bal | yes | 96% | 24,600 |
| #120 | coalescing-bal | yes | 66% | 114,195 |
| #121 | random-bal | yes | 88% | 9,423 |
| #122 | random2-bal | yes | 85% | 10,410 |
| #123 | binary-bal | yes | 55% | 3,106 |
| #124 | binary2-bal | yes | 51% | 6,136 |
| #125 | realloc-bal | yes | 33% | 386 |
| #126 | realloc2-bal | yes | 30% | 25,372 |
| | **합계** | | **71%** | **2,358** |

## Perf index

```
Perf index = 43 (util) + 40 (thru) = 83/100
```

| 이슈 | 항목 | 점수 | 상태 |
| --- | --- | --- | --- |
| #127 | util | 43 / 60 | 개선 중 (이용도) |
| #128 | thru | 40 / 40 | 만점 |

## 구현 단계별 비교

| 단계 | 커밋 | 전체 util | 전체 Kops | Perf index |
| --- | --- | --- | --- | --- |
| 묵시적 가용 리스트 + first fit + realloc 최적화 | `21f043c` | 80% | 215 | 61 ~ 62 |
| 명시적 가용 리스트 (LIFO) | `2c694ea` | 71% | 2,358 | **83** |
| 명시적 + 주소 순 삽입 (실험) | `807a913` | 80% | 약 300 | 67 ~ 68 |
| 명시적 (LIFO) + tcache | `ebe65a5` | 71% | 2,418 | **83** |
| 명시적 (주소 순) + tcache (실험, 커밋 안 함) | — | 79% | 315 | 68 |

- 처리량 : 할당 블록을 건너뛰고 빈 블록만 훑어 약 11배 → thru 만점
- 이용도 : LIFO 라 최근 free 된 블록부터 써서 realloc trace 에서 크게 떨어짐 (realloc-bal 80% → 33%)
- 주소 순 : 이용도 하락 원인이 LIFO 인지 확인한 실험. 이용도는 돌아왔지만 free 마다 리스트를 훑어 처리량이 크게 떨어짐 (binary 160 · 88 Kops)
- tcache : 크기별 bin (16 ~ 128B, bin 당 7개), bin 머리 · 블록 수는 힙 맨 앞 영역, 헤더 1번 비트 = tcache 플래그 (double free 방지)
	- 같은 크기를 바로 다시 쓰는 realloc2 에서만 효과 (이용도 30% → 36%, 24,220 → 144,010 Kops)
	- 재사용이 없는 binary 에는 효과 없음 → 전체 점수는 그대로
- 다음 목표 : 이용도 (#127)
