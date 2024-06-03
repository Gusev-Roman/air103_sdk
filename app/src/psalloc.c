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

/**
* Проверка указателя на диапазон памяти, назначенный для heap
* при записи в память большего количества байт, чем выделено, заголовок следующего блока может быть поврежден
* при попытке проследовать по поврежденному указателю получим неопределенное поведение
* поэтому перед обращениями к указателям крайне желательно проверять их валидность
*/
bool isHeap(void *mem) {
    if ((mem >= _first_chunk) && (mem < (void *)((uint32_t)_first_chunk + (_heap_sz << 4))))
        return true;
    return false;
}
// TODO: make Z-record with full memory alloc'd
void init_heap() {
    MCB* zfree;
    _heap = (void *)PSRAM_ADDR_START;
    if (!_heap) {
        puts("init_heap error: not enough memory!");
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
    printf("Heap is %p, %X\n", _heap, _heap_sz << 4);
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
// TODO: find empty chunk and split
MCB* find_sized_chunk(size_t sz) {
    MCB* curr;

    // пробежимся по цепочке (включая Z, который всегда свободен)
    // если чанк свободен, проверить его размер. 
    // если тег неизвестен, сообщить об ошибке

    if (_first_chunk) { // first is not empty
        curr = (MCB*)_first_chunk;
        while (curr->_tag == 'M') {
            // check every chunk for emptiness
            if ((curr->_flag == 0) && (curr->_sz >= sz)) {
                mem_split(curr, sz);
                return curr;   // found a hole...
            }

            if (isHeap(curr->next)) curr = curr->next;      // TODO: check next for heap area!
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
                mem_split(curr, sz);                // взять чанк из Z-блока
                return curr;
            }
            else {  // Z-блок занят, либо в нем не хватает места. Сообщить об ошибке (в вышестоящей процедуре).
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
        printf("psalloc:Error:%d > %d\n", para_sz, _heap_sz); // запрос превышает даже весь heap
        return NULL;
    }
    curr = find_sized_chunk(para_sz);          // находим чанк соответственного размера
#ifdef DEBUG
    printf("find_sized_chunk() returns %p\n", curr);
#endif
    if (!curr) {
        printf("psalloc:Error: no memory\n");   // не нашлось непрерывного пространства нужного размера
        return NULL;
    }
    //printf("Tag of found area is %c\n", curr->_tag);

    curr->_flag = 8;    // mark used

    if (curr->_tag == 'Z') {                // особый случай: выделено все оставшееся место!
        printf("Z-tag in use!..\n");
        curr->_sz = para_sz;
        //curr->prev = _last_chunk;           // последний блок не может быть предыдущим для какого-либо
        //curr->next = ((MCB*)(_free_area)) + 1 + para_sz;  // последний не имеет next
        _free_area = curr->next;
        _last_chunk = curr;     // last but not first
        _free_sz -= (para_sz + 1);  // space correction
    }
    return (void*)(curr + 1);  // point to user area
}
void *pscalloc(size_t nmemb, size_t lsize) {
    return psalloc(nmemb*lsize);
}
/*
// Coalescence to next
// chunk: chunk ptr that must be freed
 * TODO: рекурсия не нужна, если чекаем в обе стороны
 */
int coalescence(MCB* chunk) {

    if (chunk) {
        if (chunk->next && (chunk->next->_flag == 0)) {
            // it can poi!
            if (chunk->next->_tag == 'M') coalescence(chunk->next);  // recurse it

            chunk->_sz += chunk->next->_sz + 1;     // _sz - размер чанка БЕЗ заголовка! Минимум 1...
            chunk->_tag = chunk->next->_tag;
            chunk->next = chunk->next->next;
            if (chunk->next) chunk->next->prev = chunk;
            return 1;
        }
    // check for prev=NULL
    else if(chunk->prev && (chunk->prev->_flag == 0)){	// join backward
#ifdef DEBUG
	    printf("join with previous chunk");
#endif
	    chunk->prev->_sz += chunk->_sz + 1;
	    chunk->prev->next = chunk->next;
	    if(chunk->next) chunk->next->prev = chunk->prev;
	    return 1;
	}
    }
    else {
        puts("coalescence() error: wrong chunk!");
    }
    return 0;
}

// free block from PSRAM
// TODO: why coalescence() not run? - we need backward check!
void psfree(void* memo) {
    MCB* blk = (MCB*)memo;
    blk--;  // point to MCB
    // first check if block in PSRAM
    if ((blk >= (MCB *)_first_chunk) && (blk < (MCB*)((uint32_t)_first_chunk + (_heap_sz << 4)))) {
        //printf("free(%p)\n", blk);
        // get link to MCB if BLK not points to 'MZ'
        if (blk->_tag == 'M') {
            if (!blk->_flag) {
                printf("block %p is not allocated\n", blk);
                return;
            }
            blk->_flag = 0;            // just mark it freed
            _free_sz += blk->_sz;      // increase _free_sz
            coalescence(blk);          // join chunk with next one (but next is occupied! maybe try coalescence whth previous?)
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
    }
    else {
        puts("not in range, heap corrupted?");
    }
}
/*
 * учесть in = NULL, bytes = 0 и уменьшение размера
 */
void* psrealloc(void* in, size_t bytes) {
    MCB* old = (MCB*)in;
    old--;
    char* re = (char*)psalloc(bytes);
    if (re) {
        memcpy(re, in, (old->_sz) << 4);
        psfree((MCB*)in);
    }
    return re;
}

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

/*
 * Если _pentry = NULL, изучаем первый чанк
 * иначе изучаем текущий и сдвигаем _pentry на следующий
 */
int _heapwalk(_HEAPINFO *_EntryInfo){
    MCB* curr;

    if(!_EntryInfo->_pentry){   // is NULL, check first entry
        if(!_first_chunk) return _HEAPBADBEGIN;
        // now check _first_chunk for PSRAM area
        if((_first_chunk < PSRAM_ADDR_START) || (_first_chunk > PSRAM_ADDR_START + PSRAM_SIZE))
            return _HEAPBADBEGIN;
        curr = (MCB*)_first_chunk;
        _EntryInfo->_pentry = _first_chunk;
        if(curr->_tag == 'Z') return _HEAPEMPTY;
        if(curr->_tag == 'M'){	 		// chunk is used, report it!
            // go to next element
            //_EntryInfo->_pentry = curr->next;
            _EntryInfo->_size = curr->_sz;
            _EntryInfo->_useflag = (curr->_flag == 0) ? _FREEENTRY : _USEDENTRY;
            return _HEAPOK;
        }
    }
    // study NEXT entry (if valid)
    if(_EntryInfo->_pentry < PSRAM_ADDR_START) return _HEAPBADNODE;
    if(_EntryInfo->_pentry > (PSRAM_ADDR_START + PSRAM_SIZE)) return _HEAPBADNODE;

    curr = (MCB *)_EntryInfo->_pentry;
    _EntryInfo->_pentry = curr->next;  // check next chunk
    curr = curr->next;
    if(curr->_tag == 'Z'){
        _EntryInfo->_size = curr->_sz;
        _EntryInfo->_useflag = _FREEENTRY;
        return _HEAPEND; // Z-tag reached
    }
    else if(curr->_tag == 'M'){			// it CAN be free as well
        _EntryInfo->_size = curr->_sz;
        _EntryInfo->_useflag = (curr->_flag == 0) ? _FREEENTRY : _USEDENTRY;
    }
    else return _HEAPBADNODE; // tag is not M or Z
    return _HEAPOK;
}
