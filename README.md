# 2026 Krafton Jungle

크래프톤 정글 과정 동안의 **주차별 과제 코드 · 공부 노트 · 블로그 글** 저장소.
주차별 진척도는 [깃허브 프로젝트 — 크래프톤 정글 2026 학습 기록](https://github.com/users/Jongeume/projects/1) 에서 봅니다.

## 📂 폴더 구조

```
krafton-jungle-2026/
├── weekly/                          # 주차별 과제 코드
│   ├── week02-03/                   # 자료구조 & 알고리즘 (Python)
│   │   ├── week2/
│   │   │   ├── 1. basic/            # 기본 15문제
│   │   │   └── 2. advanced/         # 심화 5문제
│   │   └── week3/
│   │       ├── 1. basic/            # 기본 9문제
│   │       └── 2. advanced/         # 심화 3문제
│   ├── week4/                       # C 자료구조 구현
│   │   └── Data-Structures/
│   │       ├── Linked_List/         # Q1 ~ Q7
│   │       ├── Stack_and_Queue/     # Q1 ~ Q7
│   │       ├── Binary_Tree/         # Q1 ~ Q8
│   │       └── Binary_Search_Tree/  # Q1 ~ Q5
│   ├── week5/                       # C 메모리 버그 gdb 디버깅
│   │   ├── challenges/              # 01_use_after_free ~ 20_vector_stale_pointer
│   │   └── scripts/
│   └── week6/
│       └── malloc-lab/              # 동적 메모리 할당기 구현
├── study-notes/                     # 책 · 개념 공부 노트
│   └── csapp/
│       └── 09-virtual-memory/       # CS:APP 9장 가상메모리
├── blog/                            # 블로그 발행용 글
│   └── _posts/
└── README.md
```

| 폴더 | 언어 | 문제 수 | 비고 |
|---|---|---|---|
| [`weekly/week02-03/week2`](weekly/week02-03/week2) | Python | 15 + 5 | 문자열 · 배열부터 재귀 · 백트래킹 · 정렬까지 |
| [`weekly/week02-03/week3`](weekly/week02-03/week3) | Python | 9 + 3 | 트리 · 그래프 · DP · 그리디, 위상 정렬 · LCS · 다익스트라 |
| [`weekly/week4`](weekly/week4) | C | 27 | 연결 리스트 · 스택 / 큐 · 이진 트리 · BST 를 C 로 구현 |
| [`weekly/week5`](weekly/week5) | C | 20 | 메모리 버그를 gdb 로 찾고 고치기 (분석 README 01 ~ 07) |
| [`weekly/week6`](weekly/week6) | C | — | malloc-lab — 진행 중 |
| [`study-notes/csapp`](study-notes/csapp) | — | — | CS:APP 9장 9.1 ~ 9.6 |

## 📝 study-notes
- [CS:APP 9장 가상메모리](study-notes/csapp/09-virtual-memory/README.md)

## ✍️ blog
- _(작성 예정)_

---

### 컨벤션
- 브랜치 : 주차 과제는 `weekN`, 공부 노트는 `cs/weekN` → `main` 으로 PR
- 챕터 · 글 폴더는 `01-`, `02-` 번호 접두사 + 영문 슬러그 (읽는 순서대로 정렬)
- 각 폴더의 `README.md` 가 그 안의 목차 역할
- 이미지는 `assets/`, 실습 코드는 `code/`
