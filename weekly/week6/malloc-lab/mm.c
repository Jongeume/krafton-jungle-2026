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

// 빈 가용 리스트
static char *heap_listp;

/* 협조 함수 */
static void *coalesce(void *bp)
{
    size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    size_t size = GET_SIZE(HDRP(bp));

    if (prev_alloc && next_alloc)
    {
        // Case 1
        return bp;
    }
    else if (prev_alloc && !next_alloc)
    {
        // Case 2
        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
    }
    else if (!prev_alloc && next_alloc)
    {
        // Case 3
        size += GET_SIZE(HDRP(PREV_BLKP(bp)));
        PUT(FTRP(bp), PACK(size, 0));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
    }
    else
    {
        // Case 4
        size += GET_SIZE(HDRP(PREV_BLKP(bp))) + GET_SIZE(FTRP(NEXT_BLKP(bp)));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
    }

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

    // 첫 가용 블록
    if (extend_heap(CHUNKSIZE / WSIZE) == NULL)
        return -1;
    return 0;
}

// find_fit - first fit 방식
static void *find_fit(size_t asize)
{
    void *bp;

    for (bp = heap_listp; GET_SIZE(HDRP(bp)) > 0; bp = NEXT_BLKP(bp))
    {
        if (!GET_ALLOC(HDRP(bp)) && (asize <= GET_SIZE(HDRP(bp))))
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
    // csize = GET_SIZE(bp-4)
    size_t csize = GET_SIZE(HDRP(bp));

    // 최소블록 = 16바이트
    // 분할 후, 블록의 나머지가 최소 블록 크기와 같거나 크다면, 블록 분할.
    if ((csize - asize) >= (2 * DSIZE))
    {
        PUT(HDRP(bp), PACK(asize, 1));
        PUT(FTRP(bp), PACK(asize, 1));
        bp = NEXT_BLKP(bp);
        PUT(HDRP(bp), PACK(csize - asize, 0));
        PUT(FTRP(bp), PACK(csize - asize, 0));
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

    int newsize = ALIGN(size + SIZE_T_SIZE);
    void *p = mem_sbrk(newsize);
    if (p == (void *)-1)
        return NULL;
    else
    {
        *(size_t *)p = size;
        return (void *)((char *)p + SIZE_T_SIZE);
    }
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
    /*
    [제자리 할당 최적화]
    1. 기존의 영역이랑 크기가 같을 때. -> 값복사 X, 포인터 그대로
    2. 기존의 영역보다 크기가 작을 때. -> 값복사 X, 포인터 그대로, 블록크기 줄이기.
    */
    void *oldptr = ptr;
    void *newptr;
    size_t copySize;

    newptr = mm_malloc(size);
    if (newptr == NULL)
        return NULL;
    copySize = *(size_t *)((char *)oldptr - SIZE_T_SIZE);
    if (size < copySize)
        copySize = size;
    memcpy(newptr, oldptr, copySize);
    mm_free(oldptr);
    return newptr;
}
