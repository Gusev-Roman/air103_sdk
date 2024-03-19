// psalloc.h

#pragma once

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

// ----- main functions
void* psalloc(size_t bytes);
void* psrealloc(void* in, size_t bytes);
void psfree(void* memo);
// ---- initialization
void init_heap();
// ---- integrity diagnostics
bool isHeap(void *mem);
void heap_walk();
// ---- private functions
uint8_t mem_split(MCB* chunk, size_t reqd);
MCB* find_sized_chunk(size_t sz);
int coalescence(MCB* chunk);

