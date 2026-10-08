# 6주차 학습 과제 — 가상메모리 · 동적 메모리 할당

> 진척도 : [#130](https://github.com/Jongeume/krafton-jungle-2026/issues/130) (하위 이슈 [#95](https://github.com/Jongeume/krafton-jungle-2026/issues/95) ~ [#99](https://github.com/Jongeume/krafton-jungle-2026/issues/99))
> 바탕 : [CS:APP 9장 노트](../csapp/09-virtual-memory/README.md) · 교재 · [malloc-lab 실험 기록](../../weekly/week6/malloc-lab/README.md)

| 이슈 | 주제 | 노트 | 교재 |
| --- | --- | --- | --- |
| [#95](https://github.com/Jongeume/krafton-jungle-2026/issues/95) | 가상메모리 & 페이징 | [01-virtual-memory-and-paging](01-virtual-memory-and-paging.md) | 9.1 ~ 9.7 |
| [#96](https://github.com/Jongeume/krafton-jungle-2026/issues/96) | 동적메모리할당 (힙 · sbrk · malloc · free) | [02-dynamic-memory-allocation](02-dynamic-memory-allocation.md) | 9.9.1 ~ 9.9.3 |
| [#97](https://github.com/Jongeume/krafton-jungle-2026/issues/97) | 메모리 단편화 | [03-fragmentation](03-fragmentation.md) | 9.9.4 · 9.9.10 |
| [#98](https://github.com/Jongeume/krafton-jungle-2026/issues/98) | 메모리 할당 정책 (first · next · best fit) | [04-placement-policies](04-placement-policies.md) | 9.9.7 |
| [#99](https://github.com/Jongeume/krafton-jungle-2026/issues/99) | implicit · explicit free list | [05-implicit-and-explicit-free-lists](05-implicit-and-explicit-free-lists.md) | 9.9.6 · 9.9.13 |

- 읽는 순서 : #95 (가상메모리가 힙을 준다) → #96 (힙을 나눠 주는 할당기) → #97 (나눠 주다 생기는 손해) → #98 (어느 블록을 고르나) → #99 (빈 블록을 어떻게 엮나)
- 다섯 주제를 꿰는 한 줄 : **처리량 ↔ 이용도는 하나를 고르는 게 아니라 균형점을 찾는 문제**
- 그림은 `assets/`
