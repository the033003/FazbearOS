#pragma once

#include <stddef.h>
#include <stdint.h>

#define PAGE_SIZE 4096

struct memory_info {
    uint64_t total_memory;
    uint64_t usable_memory;
    uint64_t reserved_memory;
};

void memory_init(uint64_t multiboot_information);

const struct memory_info* memory_get_info(void);

void* page_alloc(void);
void page_free(void* address);

uint64_t page_total(void);
uint64_t page_free_count(void);
