#include "heap.h"

#include "memory.h"

#include <stdint.h>

#define HEAP_MAGIC 0xFABA1234u

struct heap_block {
    uint32_t magic;
    uint32_t free;

    size_t size;

    struct heap_block* next;
    struct heap_block* previous;
};

static struct heap_block* heap_head;

static size_t allocated_bytes;
static size_t free_bytes;

static size_t align_up(
    size_t value
)
{
    return (
        value + 15
    ) & ~(size_t)15;
}

static void split_block(
    struct heap_block* block,
    size_t requested
)
{
    if (block->size <
        requested +
        sizeof(struct heap_block) +
        16) {

        return;
    }

    uint8_t* address =
        (uint8_t*)block;

    struct heap_block* next =
        (struct heap_block*)(
            address +
            sizeof(struct heap_block) +
            requested
        );

    next->magic = HEAP_MAGIC;
    next->free = 1;
    next->size =
        block->size -
        requested -
        sizeof(struct heap_block);

    next->next = block->next;
    next->previous = block;

    if (block->next != 0) {
        block->next->previous = next;
    }

    block->next = next;
    block->size = requested;

    free_bytes +=
        next->size;

    if (free_bytes >=
        sizeof(struct heap_block)) {

        free_bytes -=
            sizeof(struct heap_block);
    }
}

static void merge_next(
    struct heap_block* block
)
{
    struct heap_block* next =
        block->next;

    if (next == 0 ||
        !next->free) {
        return;
    }

    block->size +=
        sizeof(struct heap_block) +
        next->size;

    block->next =
        next->next;

    if (block->next != 0) {
        block->next->previous =
            block;
    }
}

void heap_init(void)
{
    heap_head = 0;
    allocated_bytes = 0;
    free_bytes = 0;

    /*
     * The heap grows lazily from page_alloc().
     */
}

void* kmalloc(size_t size)
{
    if (size == 0) {
        return 0;
    }

    size = align_up(size);

    struct heap_block* block =
        heap_head;

    while (block != 0) {
        if (block->magic == HEAP_MAGIC &&
            block->free &&
            block->size >= size) {

            block->free = 0;

            if (free_bytes >= block->size) {
                free_bytes -= block->size;
            }

            allocated_bytes +=
                block->size;

            split_block(
                block,
                size
            );

            return (uint8_t*)block +
                sizeof(struct heap_block);
        }

        block = block->next;
    }

    /*
     * Allocate enough whole pages to contain
     * the requested block.
     */
    size_t total =
        sizeof(struct heap_block) +
        size;

    size_t pages =
        (total + PAGE_SIZE - 1) /
        PAGE_SIZE;

    struct heap_block* new_block =
        0;

    for (size_t i = 0;
         i < pages;
         i++) {

        void* page =
            page_alloc();

        if (page == 0) {
            return 0;
        }

        if (i == 0) {
            new_block =
                (struct heap_block*)page;
        }
    }

    new_block->magic =
        HEAP_MAGIC;

    new_block->free = 0;

    new_block->size =
        pages * PAGE_SIZE -
        sizeof(struct heap_block);

    new_block->next = 0;

    new_block->previous = 0;

    if (heap_head == 0) {
        heap_head = new_block;
    } else {
        struct heap_block* tail =
            heap_head;

        while (tail->next != 0) {
            tail = tail->next;
        }

        tail->next = new_block;
        new_block->previous = tail;
    }

    allocated_bytes +=
        new_block->size;

    return (uint8_t*)new_block +
        sizeof(struct heap_block);
}

void* kcalloc(
    size_t count,
    size_t size
)
{
    if (count == 0 ||
        size == 0) {
        return 0;
    }

    if (count >
        ((size_t)-1) / size) {
        return 0;
    }

    size_t total =
        count * size;

    uint8_t* memory =
        (uint8_t*)kmalloc(total);

    if (memory == 0) {
        return 0;
    }

    for (size_t i = 0;
         i < total;
         i++) {

        memory[i] = 0;
    }

    return memory;
}

void kfree(void* pointer)
{
    if (pointer == 0) {
        return;
    }

    struct heap_block* block =
        (struct heap_block*)(
            (uint8_t*)pointer -
            sizeof(struct heap_block)
        );

    if (block->magic != HEAP_MAGIC) {
        return;
    }

    if (block->free) {
        return;
    }

    block->free = 1;

    if (allocated_bytes >= block->size) {
        allocated_bytes -= block->size;
    }

    free_bytes += block->size;

    merge_next(block);

    if (block->previous != 0 &&
        block->previous->free) {

        merge_next(block->previous);
    }
}

void* krealloc(
    void* pointer,
    size_t size
)
{
    if (pointer == 0) {
        return kmalloc(size);
    }

    if (size == 0) {
        kfree(pointer);
        return 0;
    }

    struct heap_block* block =
        (struct heap_block*)(
            (uint8_t*)pointer -
            sizeof(struct heap_block)
        );

    if (block->magic != HEAP_MAGIC) {
        return 0;
    }

    if (block->size >= size) {
        return pointer;
    }

    void* new_pointer =
        kmalloc(size);

    if (new_pointer == 0) {
        return 0;
    }

    uint8_t* source =
        (uint8_t*)pointer;

    uint8_t* destination =
        (uint8_t*)new_pointer;

    for (size_t i = 0;
         i < block->size;
         i++) {

        destination[i] = source[i];
    }

    kfree(pointer);

    return new_pointer;
}

size_t heap_used(void)
{
    return allocated_bytes;
}

size_t heap_free(void)
{
    return free_bytes;
}
