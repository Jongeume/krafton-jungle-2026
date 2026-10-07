/*
 * mm-naive.c - The fastest, least memory-efficient malloc package.
 *
 * In this naive approach, a block is allocated by simply incrementing
 * the brk pointer.  A block is pure payload. There are no headers or
 * footers.  Blocks are never coalesced or reused. Realloc is
 * implemented directly using mm_malloc and mm_free.
 *
 * NOTE TO STUDENTS: Replace this header comment with your own header
 * comment that gives a high level description of your solution.
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "mm.h"
#include "memlib.h"

/*********************************************************
 * NOTE TO STUDENTS: Before you do anything else, please
 * provide your team information in the following struct.
 ********************************************************/
team_t team = {
    /* Team name */
    "No.1 team",
    /* First member's full name */
    "Jongeume",
    /* First member's email address */
    "Jongeume@gmail.com",
    /* Second member's full name (leave blank if none) */
    "",
    /* Second member's email address (leave blank if none) */
    ""};

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8 // 정렬

/* Basic constants and macros */
#define WSIZE 4                           // Word, 헤더, 풋터 size (bytes)
#define DSIZE 8                           // Double word size (bytes)
#define CHUNKSIZE (1 << 12)               // 힙을 한 번에 늘리는 크기 (0000 0000 0000 0001   ->   0001 0000 0000 0000) 2^12 = 4096, 4KB
#define MAX(x, y) ((x) > (y) ? (x) : (y)) // x,y 비교 max값

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT - 1)) & ~0x7) // 하위 3비트 반올림 8배수 정렬

#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

/* Pack - 크기와 할당 비트를 한 워드로 */
#define PACK(size, alloc) ((size) | (alloc)) // 크기와 할당 비트를 한 워드로

/* 주소 p의 워드를 읽기/쓰기 */
#define GET(p) (*(unsigned int *)(p))              // 음수없이 표현 0 ~ 4,2..,..
#define PUT(p, val) (*(unsigned int *)(p) = (val)) // 음수없이 표현 0 ~ 4,2..,..

/* 주소 p에 있는 헤더 또는 풋터의 size와 할당 비트를 리턴 */
#define GET_SIZE(p) (GET(p) & ~0x7) // 크기만 하위 3비트 지움 GET(p) & ~0x7
#define GET_ALLOC(p) (GET(p) & 0x1) // 맨 아래 비트만 GET(p) & 0x1

/* 블록 포인터 bp가 주어지면, 각각 블록 헤더와 풋터를 가르키는 포인터를 리턴 */
#define HDRP(bp) ((char *)(bp) - WSIZE)                      // = bp - 4
#define FTRP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE) // = bp + GET_SIZE(HDRP(bp)) - 8

/* 다음과 이전 블록의 블록 포인터를 각각 리턴 */
#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE(((char *)(bp) - WSIZE))) // = bp + GET_SIZE(HDRP(bp))
#define PREV_BLKP(bp) ((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE))) // = bp - 이전 블록 크기 GET_SIZE(bp-8)

/* === 명시적 가용 리스트 매크로 === */
#define PRED(bp) (*(char **)(bp))                   // 앞 빈블록
#define SUCC(bp) (*(char **)((char *)(bp) + WSIZE)) // 뒤 빈블록

// 힙의 시작점
static char *heap_listp;

// 리스트의 첫 빈블록 - 비어있으면 NULL
static char *free_listp;

static void insert_free(void *bp)
{
    void *stopBp = free_listp;
    void *preBp = NULL;

    // 밑으로 큰 주소가 쌓여야함.
    while (stopBp != NULL && stopBp < bp)
    {
        preBp = stopBp;
        stopBp = SUCC(stopBp);
    }

    if (preBp == NULL && stopBp == NULL)
    {
        // 1. 리스트 빔
        PRED(bp) = NULL;
        SUCC(bp) = NULL;
        free_listp = bp;
    }
    else if (preBp == NULL)
    {
        // 2. 맨 위에 들어감
        PRED(bp) = NULL;
        SUCC(bp) = free_listp;
        if (free_listp != NULL)
            PRED(free_listp) = bp;
        free_listp = bp;
    }
    else if (stopBp == NULL)
    {
        // 4. 맨 아래 들어감
        PRED(bp) = preBp;
        SUCC(bp) = NULL;
        SUCC(preBp) = bp;
    }
    else
    {
        // 3. 중간에 들어감
        PRED(bp) = preBp;
        SUCC(preBp) = bp;
        SUCC(bp) = stopBp;
        PRED(stopBp) = bp;
    }
}

static void remove_free(void *bp)
{
    char *prev = PRED(bp);
    char *next = SUCC(bp);

    if (prev != NULL)
        SUCC(prev) = next;
    else
        free_listp = next;

    if (next != NULL)
        PRED(next) = prev;
}

/* 협조 함수 */
static void *coalesce(void *bp)
{
    size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    size_t size = GET_SIZE(HDRP(bp));

    // Case 1
    if (prev_alloc && next_alloc)
    {
    }
    // Case 2
    else if (prev_alloc && !next_alloc)
    {
        remove_free(NEXT_BLKP(bp));
        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
    }
    // Case 3
    else if (!prev_alloc && next_alloc)
    {
        remove_free(PREV_BLKP(bp));
        size += GET_SIZE(HDRP(PREV_BLKP(bp)));
        PUT(FTRP(bp), PACK(size, 0));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
    }
    // Case 4
    else
    {
        remove_free(NEXT_BLKP(bp));
        remove_free(PREV_BLKP(bp));
        size += GET_SIZE(HDRP(PREV_BLKP(bp))) + GET_SIZE(FTRP(NEXT_BLKP(bp)));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
    }

    insert_free(bp);
    return bp;
}

/* 새 가용 블록으로 힙 확장하기. */
static void *extend_heap(size_t words)
{
    char *bp;
    size_t size;

    size = (words % 2) ? (words + 1) * WSIZE : words * WSIZE;
    if ((long)(bp = mem_sbrk(size)) == -1) // memlib.c -  mem_sbrk c:59 ~ 70
        return NULL;

    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1));

    return coalesce(bp);
}

/*
 * mm_init - initialize the malloc package.
 * 1. mem_sbrk(4 * WSIZE) 로 4워드를 받는다
 * 2. 첫째 워드 : 0 — 정렬 패딩
 * 3. 둘째 워드 : PACK(DSIZE, 1) — 프롤로그 헤더
 * 4. 셋째 워드 : PACK(DSIZE, 1) — 프롤로그 풋터
 * 5. 넷째 워드 : PACK(0, 1) — 에필로그 헤더
 * 6. heap_listp 를 2워드 뒤로 (프롤로그 헤더 바로 뒤)
 * 7. extend_heap(CHUNKSIZE / WSIZE) 로 첫 가용 블록
 */
int mm_init(void)
{
    if ((heap_listp = mem_sbrk(4 * WSIZE)) == (void *)-1)
        return -1;
    PUT(heap_listp, 0);                            /* 패딩 정렬 */
    PUT(heap_listp + (1 * WSIZE), PACK(DSIZE, 1)); /* 프롤로그 헤더 */
    PUT(heap_listp + (2 * WSIZE), PACK(DSIZE, 1)); /* 프롤로그 풋터 */
    PUT(heap_listp + (3 * WSIZE), PACK(0, 1));     /* 에필로그 헤더 */
    heap_listp += (2 * WSIZE);

    free_listp = NULL;

    // 첫 가용 블록
    if (extend_heap(CHUNKSIZE / WSIZE) == NULL)
        return -1;
    return 0;
}

// find_fit - first fit 방식
static void *find_fit(size_t asize)
{
    void *bp;

    for (bp = free_listp; bp != NULL; bp = SUCC(bp))
    {
        if ((asize <= GET_SIZE(HDRP(bp))))
        {
            return bp;
        }
    }

    // No fit - malloc에서 힙에서 새로운 가용블록 확장시킴.
    return NULL;
}

// place
static void place(void *bp, size_t asize)
{
    remove_free(bp);
    // csize = GET_SIZE(bp-4)
    size_t csize = GET_SIZE(HDRP(bp));

    // 최소블록 = 16바이트 (헤더 4 + PRED 4 + SUCC 4 + 풋터 4, -m32)
    // 분할 후, 블록의 나머지가 최소 블록 크기와 같거나 크다면, 블록 분할.
    if ((csize - asize) >= (2 * DSIZE))
    {
        PUT(HDRP(bp), PACK(asize, 1));
        PUT(FTRP(bp), PACK(asize, 1));
        bp = NEXT_BLKP(bp);
        PUT(HDRP(bp), PACK(csize - asize, 0));
        PUT(FTRP(bp), PACK(csize - asize, 0));
        insert_free(bp);
    }
    else
    {
        PUT(HDRP(bp), PACK(csize, 1));
        PUT(FTRP(bp), PACK(csize, 1));
    }
}

/*
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{
    size_t asize;
    size_t extendsize;
    char *bp;

    // 1. size == 0
    if (size == 0)
        return NULL;

    /*
    2.
    size <= DSIZE, = 2 * DSIZE
        - 최소 16바이트 크기의 블록 구성
        - 8바이트 : 정렬 요건 만족
        - 8바이트 : 헤더와 풋터 오버헤드
    size > DSIZE,
        - 오버헤드 바이트 추가
        - 인접 8의 배수로 반올림
    */
    if (size < DSIZE)
        asize = 2 * DSIZE;
    else
        asize = DSIZE * ((size + DSIZE + DSIZE - 1) / DSIZE);

    // 3. 가용리스트에서 적절한 가용블럭 검색
    if ((bp = find_fit(asize)) != NULL)
    {
        place(bp, asize);
        return bp;
    }

    // 4. 할당기가 맞는 블럭 못 찾았다면, 힙에 새로운 가용블록 확장
    extendsize = MAX(asize, CHUNKSIZE);
    if ((bp = extend_heap(extendsize / WSIZE)) == NULL)
        return NULL;
    place(bp, asize);
    return bp;
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr)
{
    size_t size = GET_SIZE(HDRP(ptr));

    PUT(HDRP(ptr), PACK(size, 0));
    PUT(FTRP(ptr), PACK(size, 0));
    coalesce(ptr);
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size)
{

    if (ptr == NULL)
        return mm_malloc(size);

    if (size == 0)
    {
        mm_free(ptr);
        return NULL;
    }

    void *curPtr = ptr;
    void *newPtr;
    size_t curSize = GET_SIZE(HDRP(curPtr));             // 지금 블록 크기
    size_t nextSize = GET_SIZE(HDRP(NEXT_BLKP(curPtr))); // 다음 블록 크기
    size_t combinedSize = curSize + nextSize;
    size_t asize; // 원하는 블록 크기

    // 원하는 블록 크기
    if (size < DSIZE)
        asize = 2 * DSIZE;
    else
        asize = DSIZE * ((size + DSIZE + DSIZE - 1) / DSIZE);

    // 1. asize가 현재블록 보다 클 때
    if (asize > curSize)
    {
        // 다음 블록이 가용상태 and 병합블록크기(old + next)가 asize보다 크거나 같을때
        if (!GET_ALLOC(HDRP(NEXT_BLKP(curPtr))) && (combinedSize) >= asize)
        {
            newPtr = curPtr;
            remove_free(NEXT_BLKP(curPtr));
            if ((combinedSize)-asize >= 2 * DSIZE)
            {
                PUT(HDRP(newPtr), PACK(asize, 1));
                PUT(FTRP(newPtr), PACK(asize, 1));
                PUT(HDRP(NEXT_BLKP(newPtr)), PACK(combinedSize - asize, 0));
                PUT(FTRP(NEXT_BLKP(newPtr)), PACK(combinedSize - asize, 0));
                insert_free(NEXT_BLKP(newPtr));
            }
            else
            {
                PUT(HDRP(newPtr), PACK(combinedSize, 1));
                PUT(FTRP(newPtr), PACK(combinedSize, 1));
            }
        }
        // 다음 블록 할당상태
        else
        {
            newPtr = mm_malloc(size);
            if (newPtr == NULL)
                return NULL;

            memcpy(newPtr, curPtr, curSize - DSIZE);
            mm_free(curPtr);
        }
    }
    // 2. asize가 현재블록크기 보다 작을 때
    else if (asize < curSize)
    {
        // curSize − asize >= 16(최소크기) 일 때만 coalesce를 부른다
        size_t remain = curSize - asize;
        curSize = asize;
        newPtr = curPtr;

        if (remain >= 2 * DSIZE)
        {
            PUT(HDRP(newPtr), PACK(asize, 1));
            PUT(FTRP(newPtr), PACK(asize, 1));
            PUT(HDRP(NEXT_BLKP(newPtr)), PACK(remain, 0));
            PUT(FTRP(NEXT_BLKP(newPtr)), PACK(remain, 0));
            coalesce(NEXT_BLKP(newPtr));
        }
    }
    // 3. asize가 현재블록크기 와 같을 때
    else
    {
        newPtr = curPtr;
    }

    return newPtr;
}
