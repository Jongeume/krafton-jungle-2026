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

- 처리량 : 할당 블록을 건너뛰고 빈 블록만 훑어 약 11배 → thru 만점
- 이용도 : LIFO 라 최근 free 된 블록부터 써서 realloc trace 에서 크게 떨어짐 (realloc-bal 80% → 33%)
- 다음 목표 : 이용도 (#127)
