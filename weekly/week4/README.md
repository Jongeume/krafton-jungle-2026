## 📋 내 풀이 인덱스 — C로 구현한 자료구조

### 연결 리스트

| # | 구현 함수 | 내용 | ⭐ | 코드 |
| --- | --- | --- | :---: | --- |
| Q1 | `insertSortedLL` | 정렬 유지 삽입 |  | [Q1_A_LL.c](Data-Structures/Linked_List/Q1_A_LL.c) |
| Q2 | `alternateMergeLinkedList` | 번갈아 병합 |  | [Q2_A_LL.c](Data-Structures/Linked_List/Q2_A_LL.c) |
| Q3 | `moveOddItemsToBack` | 홀수를 뒤로 |  | [Q3_A_LL.c](Data-Structures/Linked_List/Q3_A_LL.c) |
| Q4 | `moveEvenItemsToBack` | 짝수를 뒤로 |  | [Q4_A_LL.c](Data-Structures/Linked_List/Q4_A_LL.c) |
| Q5 | `frontBackSplitLinkedList` | 앞뒤로 나누기 |  | [Q5_A_LL.c](Data-Structures/Linked_List/Q5_A_LL.c) |
| Q6 | `moveMaxToFront` | 최댓값을 앞으로 |  | [Q6_A_LL.c](Data-Structures/Linked_List/Q6_A_LL.c) |
| Q7 | `RecursiveReverse` | 재귀로 뒤집기 | ⭐ | [Q7_A_LL.c](Data-Structures/Linked_List/Q7_A_LL.c) |

### 스택 · 큐

| # | 구현 함수 | 내용 | ⭐ | 코드 |
| --- | --- | --- | :---: | --- |
| Q1 | `createQueueFromLinkedList · removeOddValues` | 리스트 → 큐, 홀수 제거 |  | [Q1_C_SQ.c](Data-Structures/Stack_and_Queue/Q1_C_SQ.c) |
| Q2 | `createStackFromLinkedList · removeEvenValues` | 리스트 → 스택, 짝수 제거 |  | [Q2_C_SQ.c](Data-Structures/Stack_and_Queue/Q2_C_SQ.c) |
| Q3 | `isStackPairwiseConsecutive` | 스택 쌍 연속 검사 |  | [Q3_C_SQ.c](Data-Structures/Stack_and_Queue/Q3_C_SQ.c) |
| Q4 | `reverse` | 큐 뒤집기 (스택 이용) |  | [Q4_C_SQ.c](Data-Structures/Stack_and_Queue/Q4_C_SQ.c) |
| Q5 | `recursiveReverse` | 큐 재귀로 뒤집기 |  | [Q5_C_SQ.c](Data-Structures/Stack_and_Queue/Q5_C_SQ.c) |
| Q6 | `removeUntil` | 값까지 pop |  | [Q6_C_SQ.c](Data-Structures/Stack_and_Queue/Q6_C_SQ.c) |
| Q7 | `balanced` | 괄호 균형 | ⭐ | [Q7_C_SQ.c](Data-Structures/Stack_and_Queue/Q7_C_SQ.c) |

### 이진 트리

| # | 구현 함수 | 내용 | ⭐ | 코드 |
| --- | --- | --- | :---: | --- |
| Q1 | `identical` | 두 트리가 같은가 |  | [Q1_E_BT.c](Data-Structures/Binary_Tree/Q1_E_BT.c) |
| Q2 | `maxHeight` | 최대 높이 | ⭐ | [Q2_E_BT.c](Data-Structures/Binary_Tree/Q2_E_BT.c) |
| Q3 | `countOneChildNodes` | 자식 하나인 노드 수 |  | [Q3_E_BT.c](Data-Structures/Binary_Tree/Q3_E_BT.c) |
| Q4 | `sumOfOddNodes` | 홀수 노드 합 |  | [Q4_E_BT.c](Data-Structures/Binary_Tree/Q4_E_BT.c) |
| Q5 | `mirrorTree` | 좌우 뒤집기 |  | [Q5_E_BT.c](Data-Structures/Binary_Tree/Q5_E_BT.c) |
| Q6 | `printSmallerValues` | 작은 값 출력 |  | [Q6_E_BT.c](Data-Structures/Binary_Tree/Q6_E_BT.c) |
| Q7 | `smallestValue` | 최솟값 |  | [Q7_E_BT.c](Data-Structures/Binary_Tree/Q7_E_BT.c) |
| Q8 | `hasGreatGrandchild` | 증손자가 있는 노드 |  | [Q8_E_BT.c](Data-Structures/Binary_Tree/Q8_E_BT.c) |

### 이진 탐색 트리

| # | 구현 함수 | 내용 | ⭐ | 코드 |
| --- | --- | --- | :---: | --- |
| Q1 | `levelOrderTraversal` | 레벨 순회 (큐) |  | [Q1_F_BST.c](Data-Structures/Binary_Search_Tree/Q1_F_BST.c) |
| Q2 | `inOrderTraversal` | 중위 순회 (반복) |  | [Q2_F_BST.c](Data-Structures/Binary_Search_Tree/Q2_F_BST.c) |
| Q3 | `preOrderIterative` | 전위 순회 (반복) |  | [Q3_F_BST.c](Data-Structures/Binary_Search_Tree/Q3_F_BST.c) |
| Q4 | `postOrderIterativeS1` | 후위 순회 (스택 1개) |  | [Q4_F_BST.c](Data-Structures/Binary_Search_Tree/Q4_F_BST.c) |
| Q5 | `postOrderIterativeS2 · removeNodeFromTree` | 후위 순회 (스택 2개) · 노드 삭제 | ⭐ | [Q5_F_BST.c](Data-Structures/Binary_Search_Tree/Q5_F_BST.c) |

---

# 📘 Docker + VSCode DevContainer 기반 C 개발 환경 구축 가이드

이 문서는 **Windows**와 **macOS** 사용자가 Docker와 VSCode DevContainer 기능을 활용하여 C 개발 및 디버깅 환경을 빠르게 구축할 수 있도록 도와줍니다.

---

## 1. Docker란 무엇인가요?

**Docker**는 애플리케이션을 어떤 컴퓨터에서든 **동일한 환경에서 실행**할 수 있게 도와주는 **가상화 플랫폼**입니다.  

Docker는 다음 구성요소로 이루어져 있습니다:

- **Docker Engine**: 컨테이너를 실행하는 핵심 서비스
- **Docker Image**: 컨테이너 생성에 사용되는 템플릿 (레시피 📃)
- **Docker Container**: 이미지를 기반으로 생성된 실제 실행 환경 (요리 🍜)

### ✅ AWS EC2와의 차이점

| 구분 | EC2 같은 VM | Docker 컨테이너 |
|------|-------------|-----------------|
| 실행 단위 | OS 포함 전체 | 애플리케이션 단위 |
| 실행 속도 | 느림 (수십 초 이상) | 매우 빠름 (거의 즉시) |
| 리소스 사용 | 무거움 | 가벼움 |

---

## 2. VSCode DevContainer란 무엇인가요?

**DevContainer**는 VSCode에서 Docker 컨테이너를 **개발 환경**처럼 사용할 수 있게 해주는 기능입니다.

- 코드를 실행하거나 디버깅할 때 **컨테이너 내부 환경에서 동작**
- 팀원 간 **환경 차이 없이 동일한 개발 환경 구성** 가능
- `.devcontainer` 폴더에 정의된 설정을 VSCode가 읽어 자동 구성

---

## 3. Docker Desktop 설치하기

1. Docker 공식 사이트에서 설치 파일 다운로드:  
   👉 [https://www.docker.com/products/docker-desktop](https://www.docker.com/products/docker-desktop)

2. 설치 후 Docker Desktop 실행  
   - Windows: Docker 아이콘이 트레이에 떠야 함  
   - macOS: 상단 메뉴바에 Docker 아이콘 확인

---

## 4. 프로젝트 파일 다운로드 (히스토리 없이)

터미널(CMD, PowerShell, zsh 등)에서 아래 명령어로 프로젝트 폴더만 내려받습니다:

```bash
git clone --depth=1 https://github.com/krafton-jungle/data_structures_docker.git
```

- `--depth=1` 옵션은 git commit 히스토리를 생략하고 **최신 파일만 가져옵니다.**

### 📂 다운로드 후 폴더 구조 설명

```
week4/
├── .devcontainer/
│   ├── devcontainer.json      # VSCode에서 컨테이너 환경 설정
│   └── Dockerfile             # C 개발 환경 이미지 정의
│
├── .vscode/
│   ├── launch.json            # 디버깅 설정 (F5 실행용)
│   └── tasks.json             # 컴파일 자동화 설정
│
├── Data-Structures/
│   ├── Binary_Search_Tree/
│   ├── Binary_Tree/
│   ├── Linked_List/
│   │   ├── Q1_A_LL.c ~ Q7_A_LL.c       # 연결 리스트 문제 C 코드
│   │   └── Linked Lists Questions.pdf # 문제 설명 문서
│   └── Stack_and_Queue/
│       ├── Q1_C_SQ.c ~ Q7_C_SQ.c       # 스택/큐 문제 C 코드
│       └── Stack and Queues Questions.pdf # 문제 설명 문서
│
└── README.md  # 설치 및 사용법 설명 문서
```

> ✅ 각 디렉터리는 자료구조 주제별로 문제 파일들이 정리되어 있습니다.

---

## 5. VSCode에서 해당 프로젝트 폴더 열기

1. VSCode를 실행
2. `파일 → 폴더 열기`로 방금 클론한 `data_structures_docker` 폴더를 선택
---

## 6. 개발 컨테이너: 컨테이너에서 열기


1. VSCode에서 `Ctrl+Shift+P` (Windows/Linux) 또는 `Cmd+Shift+P` (macOS)를 누릅니다.
2. 명령어 팔레트에서 `Dev Containers: Reopen in Container`를 선택합니다.
3. 이후 컨테이너가 자동으로 실행되고 빌드됩니다. 처음 컨테이너를 열면 빌드하는 시간이 오래걸릴 수 있습니다. 빌드 후, 프로젝트가 **컨테이너 안에서 실행됨**.

---

## 7. C 파일에 브레이크포인트 설정 후 디버깅 (F5)
이제 본격적으로 문제를 풀 시간입니다. `README.md` 파일을 참조하셔서 Linked List -> Stack and Queue -> Binary Tree -> Binary Search Tree 순으로 문제를 풀어보세요. 각 문제 폴더에는 pdf형태로 문제 설명이 있습니다.

C 언어로 문제를 풀다가 디버깅이 필요하시면 소스코드에 BreakPoint를 설정한 뒤에 키보드에서 `F5`를 눌러 디버깅을 시작할 수 있습니다.   
* 참고로 변수, 메모리, 스택, 출력 등을 VSCode에서 확인할 수도 있습니다.

---



## 8. 새로운 Git 리포지토리에 Commit & Push 하기

금주 프로젝트를 개인 Git 리포와 같은 다른 리포지토리에 업로드하려면, 기존 Git 연결을 제거하고 새롭게 초기화해야 합니다.

### ✅ 완전히 새로운 Git 리포로 업로드하는 방법

아래 명령어를 순서대로 실행하세요:

```bash
rm -rf .git
git init
git remote add origin https://github.com/myusername/my-new-repo.git
git add .
git commit -m "Clean start"
git push -u origin main
```

### 📌 설명:

- `rm -rf .git`: 기존 Git 기록과 연결을 완전히 삭제합니다.
- `git init`: 현재 폴더를 새로운 Git 리포지토리로 초기화합니다.
- `git remote add origin ...`: 새로운 리포지토리 주소를 origin으로 등록합니다.
- `git add .` 및 `git commit`: 모든 파일을 커밋합니다.
- `git push`: 새로운 리포에 최초 업로드(Push)합니다.

이 과정을 거치면 기존 리포와의 연결은 완전히 제거되고, **새로운 독립적인 프로젝트로 관리**할 수 있습니다.

## 🎉 끝

이제 Docker와 DevContainer를 활용한 C 개발 환경이 완성되었습니다.

- (주의) 위 내용은 처음 설치하는 사람을 기준으로 작성된 내용입니다. Data-Structures 폴더에서 있는 프로젝트를 반복적으로 개발할 경우 5에서 7장의 내용만 반복하시면 됩니다.
- 어떤 운영체제에서든 동일한 환경으로 개발 가능  
- VSCode 내에서 코드 작성, 컴파일, 디버깅까지 한 번에 가능
---
