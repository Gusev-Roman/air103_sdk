// psalloc.h

#pragma once

#define PSRAM_SIZE 8*1024*1024
#pragma pack(push, 1)

struct _tinymcb {
    char _tag;		   // 1
    uint16_t _flag;	   // 3
    struct _tinymcb* next; // 7
    struct _tinymcb* prev; // 11
    size_t _sz;		   // 15
    char _hash[1];	   // 16
};

#pragma pack(pop)

typedef struct _tinymcb MCB;

/* Return codes for _heapwalk()  */
#define _HEAPEMPTY (-1)
#define _HEAPOK (-2)
#define _HEAPBADBEGIN (-3)
#define _HEAPBADNODE (-4)
#define _HEAPEND (-5)
#define _HEAPBADPTR (-6)

/* Values for _heapinfo.useflag */
#define _FREEENTRY 0
#define _USEDENTRY 1

/* The structure used to walk through the heap with _heapwalk.  */
typedef struct _heapinfo {
    int *_pentry;
    size_t _size;
    int _useflag;
} _HEAPINFO;


// ----- main functions
void* psalloc(size_t bytes);
extern void *pscalloc(size_t nmemb, size_t lsize);
void* psrealloc(void* in, size_t bytes);
void psfree(void* memo);
// ---- integrity diagnostics
bool isHeap(void *mem);
void heap_walk();
int _heapwalk(_HEAPINFO *_EntryInfo);
// ---- private functions
void init_heap();
uint8_t mem_split(MCB* chunk, size_t reqd);
MCB* find_sized_chunk(size_t sz);
int coalescence(MCB* chunk);


