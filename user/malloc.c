#include "stdio.h"
#include "stdlib.h"

#define HEAP_ALIGN 64UL

struct heap_block {
    unsigned long size;
    struct heap_block *prev, *next;
    int available;
} __attribute__((aligned(HEAP_ALIGN)));

static struct heap_block *first, *last;
static unsigned long cursor, heap_end;

static unsigned long block_end(struct heap_block *b){
    return (unsigned long)(b + 1) + b->size;
}

static void insert_after(struct heap_block *prev, struct heap_block *b){
    b->prev = prev;
    b->next = prev ? prev->next : first;
    if(prev) prev->next = b;
    else first = b;
    if(b->next) b->next->prev = b;
    else last = b;
}

void *malloc(unsigned long size){
    if(size > ~0UL - (HEAP_ALIGN - 1)) return NULL;
    size = size ? (size + HEAP_ALIGN - 1) & ~(HEAP_ALIGN - 1) : HEAP_ALIGN;
    for(struct heap_block *b = first; b; b = b->next){
        if(!b->available || b->size < size) continue;
        if(b->size - size >= sizeof(*b) + HEAP_ALIGN){
            struct heap_block *rest = (void *)((char *)(b + 1) + size);
            rest->size = b->size - size - sizeof(*b);
            rest->available = 1;
            insert_after(b, rest);
            b->size = size;
        }
        b->available = 0;
        return b + 1;
    }

    long current = brk(0);
    if(current < 0) return NULL;
    unsigned long start = (unsigned long)current == heap_end ? cursor : (unsigned long)current;
    unsigned long end;
    if(__builtin_add_overflow(start, sizeof(struct heap_block), &end) ||
       __builtin_add_overflow(end, size, &end)) return NULL;
    if(end > (unsigned long)current){
        long result = brk(end);
        if(result < 0) return NULL;
        current = result;
    }
    heap_end = current;
    cursor = end;
    struct heap_block *b = (void *)start;
    b->size = size;
    b->available = 0;
    insert_after(last, b);
    return b + 1;
}

static void merge_next(struct heap_block *b){
    struct heap_block *next = b->next;
    if(!next || !next->available || block_end(b) != (unsigned long)next) return;
    b->size += sizeof(*next) + next->size;
    b->next = next->next;
    if(b->next) b->next->prev = b;
    else last = b;
}

void free(void *addr){
    if(!addr) return;
    struct heap_block *b = (struct heap_block *)addr - 1;
    b->available = 1;
    merge_next(b);
    if(b->prev && b->prev->available && block_end(b->prev) == (unsigned long)b){
        b = b->prev;
        merge_next(b);
    }
    if(b != last || block_end(b) != cursor || (unsigned long)brk(0) != heap_end) return;
    struct heap_block *prev = b->prev;
    unsigned long start = (unsigned long)b;
    long result = brk(start);
    if(result < 0) return;
    cursor = start;
    heap_end = result;
    last = prev;
    if(prev) prev->next = NULL;
    else first = NULL;
}
