/**
    psram.c - alloc, realloc, free for PSRAM
*/

#include <stdio.h>
#include <string.h>
#include "wm_type_def.h"
#include <wm_psram.h>
#include "psalloc.h"

void* _first_chunk = NULL;
void* _free_area;
void* _heap;
static const size_t _heap_sz = 1024 * 1024 / 2;    // 8 Mb chip used (in 16-b chunks)
size_t _free_sz;

MCB* _last_chunk;

/*
 * PTR checking is very recommended option!
 */
bool isHeap(void *mem) {
    if ((mem >= _first_chunk) && (mem < (void *)((uint32_t)_first_chunk + (_heap_sz << 4))))
        return true;
    return false;
}
// make Z-record with full memory alloc'd
void init_heap() {
    MCB* zfree;
    _heap = (void *)PSRAM_ADDR_START;
    if (!_heap) {
#ifdef DEBUG    
        printf("%s: error: not enough memory!", __FUNCTION__);
#endif        
        return;
    }
    zfree = (MCB*)_heap;
    zfree->_tag = 'Z';
    zfree->_sz = _heap_sz - 1;
    zfree->prev = NULL;
    zfree->next = NULL;
    zfree->_flag = 0;

    _first_chunk = zfree;
    // TODO: delete this!
    _free_area = _heap;
    _free_sz = _heap_sz;
#ifdef DEBUG    
    printf("%s:Heap is %p, %X\n", __FUNCTION__, _heap, _heap_sz << 4);
#endif
}

// check chunk space, if match - split it, if not - return 0
// заранее известно, что чанк не занят и что его размер не меньше, чем запрошенный!
uint8_t mem_split(MCB* chunk, size_t reqd) {
    size_t chunk_sz;
    MCB* new_chunk;

    if (chunk->_sz - reqd > 1) {  // slpit need
        chunk_sz = chunk->_sz;
        chunk->_sz = reqd;
        new_chunk = chunk + reqd + 1; // pointer to free space
        new_chunk->_tag = chunk->_tag; // M or Z
        new_chunk->_flag = 0;   // is free!
        chunk->_tag = 'M';
        new_chunk->next = chunk->next;

        new_chunk->prev = chunk;
        new_chunk->_sz = chunk_sz - reqd - 1;
        _free_sz -= (reqd + 1);
        chunk->next = new_chunk;
    }
    // else - take full chunk
    return 1;
}

/*  find empty chunk and split
 *
 */
MCB* find_sized_chunk(size_t sz) {
    MCB* curr;

    // scan MCB-chain (including Z-block, always free)
    // if chunk is free, compare its size with sz
    // if tag is not MZ, report error

    if (_first_chunk) { // first is not empty
        curr = (MCB*)_first_chunk;
        while (curr->_tag == 'M') {
            // check every chunk for emptiness
            if ((curr->_flag == 0) && (curr->_sz >= sz)) {
                mem_split(curr, sz);
                return curr;   // found a hole...
            }

            if (isHeap(curr->next)) curr = curr->next;
            else {
                puts("Error: next is corrupted!");
                return NULL;
            }
        }
        if (curr->_tag != 'Z') {
            puts("Error: Z-chunk not found!");
            return NULL;
        }
        else {
            if ((curr->_flag == 0) && (curr->_sz >= sz)) {
                mem_split(curr, sz);                // split Z-block
                return curr;
            }
            else {  // Z-block is full, report error.
                // puts("Error: no memory!");
                return NULL;
            }
        }
    }
    else {  // first chunk is not defined (только если забыли вызвать init_heap)
        if (sz + 1 <= _heap_sz) {
            curr = (MCB*)_heap;
            curr->_tag = 'Z';
            curr->_sz = _heap_sz - 1;
            curr->_flag = 0;
            curr->prev = NULL;
            curr->next = NULL;
            _first_chunk = curr;
            return  curr;
        }
    }
    return NULL;    // no memory
}

// TODO: упростить!
void* psalloc(size_t bytes) {

    MCB* curr;
    size_t para_sz = (bytes >> 4);

    if(!_first_chunk) init_heap();      // first alloc
    if (bytes % 16) para_sz++;
    if (para_sz > _heap_sz) {
#ifdef DEBUG    
        printf("%s:Error:%d > %d\n", __FUNCTION__, para_sz, _heap_sz); // запрос превышает даже весь heap
#endif        
        return NULL;
    }
    curr = find_sized_chunk(para_sz);          // finding match-sized chunk
#ifdef DEBUG
    printf("find_sized_chunk() returns %p\n", curr);
#endif
    if (!curr) {
#ifdef DEBUG    
        printf("%s:Error: no memory\n", __FUNCTION__);  // no chunk with given size or bigger
#endif        
        return NULL;
    }
    //printf("Tag of found area is %c\n", curr->_tag);

    curr->_flag = 8;    // mark used

    if (curr->_tag == 'Z') {                   // a special case: all heap is occupied!
#if defined DEBUG
        printf("%s:Z-tag in use!..\n", __FUNCTION__);
#endif        
        curr->_sz = para_sz;
        _free_area = curr->next;
        _last_chunk = curr;         // last but not first
        _free_sz -= (para_sz + 1);  // space correction
    }
    return (void*)(curr + 1);       // point to user area
}
void *pscalloc(size_t nmemb, size_t lsize) {
    return psalloc(nmemb*lsize);
}
/*
 * Coalescence to prevoius or next chunk
 * @chunk: chunk ptr that must be freed
 * Note: chunk is already checked for NULL orinvalid PTR
 * @return zero
 */
int coalescence(MCB* chunk) {

    if(chunk->prev && (chunk->prev->_flag == 0)){
        // prev's tag cannot be 'Z'
        chunk->prev->_sz += chunk->_sz + 1; // enlarge previous
        chunk->prev->next = chunk->next;
        if(chunk->next) chunk->next->prev = chunk->prev;
        chunk = chunk->prev;	// now it's here...
    }
    // even if previous is joined, checking next...
    
    if (chunk->next && (chunk->next->_flag == 0)) { // destroy next chunk
        chunk->_sz += chunk->next->_sz + 1;     // _sz - chunk size without MCB, in pages
        chunk->_tag = chunk->next->_tag;
        chunk->next = chunk->next->next;	// chunk->next->next may be invalid
    }
    return 0;
}

// free block from PSRAM

void psfree(void* memo) {
    if(!memo) return;	// nothing to free

    MCB* blk = (MCB*)memo;
    blk--;  // point to MCB
    // first check if block in PSRAM
    if ((blk >= (MCB *)_first_chunk) && (blk < (MCB*)((uint32_t)_first_chunk + (_heap_sz << 4)))) {
        //printf("free(%p)\n", blk);
        // get link to MCB if BLK not points to 'MZ'
        if (blk->_tag == 'M') {
            if (!blk->_flag) {
#ifdef DEBUG            
                printf("%s: block %p is not allocated\n", __FUNCTION__, blk);
#endif                
                return;
            }
            blk->_flag = 0;            // just mark it freed
            _free_sz += blk->_sz;      // increase _free_sz
            coalescence(blk);          // join chunk whth previous and/or with next one
        }
        else if (blk->_tag == 'Z') {
            // make prevoius elem 'Z'-elem
            blk->_flag = 0;
            if (blk->prev) {
                blk = blk->prev;
                blk->_tag = 'Z';
                _free_sz += blk->next->_sz; // increase space at end
            }
        }
        else {
#ifdef DEBUG
        printf("%s:Wrong tag, heap corrupted?\n", __FUNCTION__);
#endif
        }
    }
    else {
#ifdef DEBUG
       printf("%s:Not in range, heap corrupted?\n", __FUNCTION__);
#endif
    }
}
/*
 * TODO: учесть уменьшение размера
 */
void* psrealloc(void* in, size_t bytes) {
    if(!in) return psalloc(bytes);
    if(!bytes) { psfree(in); return NULL; }
    
    MCB* old = (MCB*)in;
    old--;
    char* re = (char*)psalloc(bytes);
    if (re) {
        memcpy(re, in, (old->_sz) << 4);
        psfree(in);
    }
    return re;
}

/*
void heap_walk() {
    MCB* curr;

    // Размер блока Z на 1 меньше, чем _free_sz, т.к. блок имеет свой заголовок
    printf("HeapWalk begin, free space is %X...\n", _free_sz << 4);
    if (!_first_chunk) {    // NULL
        goto exit;
    }
    curr = (MCB*)_first_chunk;

    while (curr->_tag == 'M') {
        printf("[%p] Tag: %c, prev:%p, next:%p, used by:%d, sz:%X\n", curr, curr->_tag, curr->prev, curr->next, curr->_flag, curr->_sz);
        if (isHeap(curr->next)) curr = curr->next;
        else {
            printf("[%p]: corrupted field: next\n", curr);
            break;
        }
    }
    printf("[%p] Tag: %c, prev:%p, next:%p, used by:%d, sz:%X\n", curr, curr->_tag, curr->prev, curr->next, curr->_flag, curr->_sz);

exit:
    puts("...stop!");
}
*/

/*
 * microsoft-style heapwalk
 * @_EntryInfo: PTR to _HEAPINFO struct
 * if _EntryInfo->_pentry is NULL, analyze first chunk in chain
 * otherwise analyse current chunk and change _EntryInfo->_pentry to next chunk
 */
int _heapwalk(_HEAPINFO *_EntryInfo){
    MCB* curr;
    int retval = _HEAPOK;

    if(!_EntryInfo) return _HEAPBADPTR;
    
    if(!_EntryInfo->_pentry){   // is NULL, check first entry
        if(!_first_chunk) return _HEAPBADBEGIN;
        // now check _first_chunk for PSRAM area
        if((_first_chunk < (void *)PSRAM_ADDR_START) || (_first_chunk > (void *)PSRAM_ADDR_START + PSRAM_SIZE))
            return _HEAPBADBEGIN;
        curr = (MCB*)_first_chunk;
        _EntryInfo->_pentry = _first_chunk;
        _EntryInfo->_size = curr->_sz << 4;
        _EntryInfo->_useflag = (curr->_flag == 0) ? _FREEENTRY : _USEDENTRY;
        
        if(curr->_tag == 'Z'){
            retval = _HEAPEMPTY;
        }
        return retval;
    }
    
    // study NEXT entry (if valid)
    if((_EntryInfo->_pentry < (int *)PSRAM_ADDR_START|| (_EntryInfo->_pentry > ((int *)PSRAM_ADDR_START + PSRAM_SIZE))))
        return _HEAPBADPTR;

    curr = (MCB *)_EntryInfo->_pentry;
    _EntryInfo->_pentry = (int *)curr->next;  // check next chunk (MCB area)
    curr = curr->next;
    if(curr->_tag == 'Z'){
        _EntryInfo->_size = curr->_sz * 16;
        _EntryInfo->_useflag = _FREEENTRY;
        return _HEAPEND; // Z-tag reached
    }
    else if(curr->_tag == 'M'){
        _EntryInfo->_size = curr->_sz * 16;
        _EntryInfo->_useflag = (curr->_flag == 0) ? _FREEENTRY : _USEDENTRY;
    }
    else return _HEAPBADNODE; // tag is not M or Z
    return _HEAPOK;
}
