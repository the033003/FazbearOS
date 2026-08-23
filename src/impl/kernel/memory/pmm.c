#include "memory.h"

#include "print.h"

#define MAX_MEMORY_BYTES (128ULL * 1024ULL * 1024ULL)
#define MAX_PAGES \
    (MAX_MEMORY_BYTES / PAGE_SIZE)

#define PAGE_BITMAP_BYTES \
    (MAX_PAGES / 8)

static uint8_t page_bitmap[PAGE_BITMAP_BYTES];

static struct memory_info memory_information;

static uint64_t total_pages;
static uint64_t free_pages;

static void bitmap_set(
    uint64_t page
)
{
    page_bitmap[page / 8] |=
        (uint8_t)(1u << (page % 8));
}

static void bitmap_clear(
    uint64_t page
)
{
    page_bitmap[page / 8] &=
        (uint8_t)~(1u << (page % 8));
}

static int bitmap_test(
    uint64_t page
)
{
    return
        (page_bitmap[page / 8] &
        (uint8_t)(1u << (page % 8))) != 0;
}

static void reserve_range(
    uint64_t start,
    uint64_t end
)
{
    if (end <= start) {
        return;
    }

    start =
        (start + PAGE_SIZE - 1) &
        ~(uint64_t)(PAGE_SIZE - 1);

    end =
        end &
        ~(uint64_t)(PAGE_SIZE - 1);

    for (
        uint64_t address = start;
        address < end;
        address += PAGE_SIZE
    ) {
        uint64_t page =
            address / PAGE_SIZE;

        if (page >= MAX_PAGES) {
            break;
        }

        if (!bitmap_test(page)) {
            bitmap_set(page);

            if (free_pages > 0) {
                free_pages--;
            }
        }
    }
}

void memory_init(
    uint64_t multiboot_information
)
{
    /*
     * Start with every page reserved.
     */
    for (size_t i = 0;
         i < sizeof(page_bitmap);
         i++) {

        page_bitmap[i] = 0xFF;
    }

    total_pages = 0;
    free_pages = 0;

    memory_information =
        (struct memory_info) {
            0
        };

    /*
     * Multiboot2 memory information parsing.
     *
     * The bootloader gives us a list of tagged
     * structures. We only care about the memory
     * map at this stage.
     */
    uint8_t* address =
        (uint8_t*)(uintptr_t)
        multiboot_information;

    uint32_t total_size =
        *(uint32_t*)address;

    address += 8;

    uint8_t* end =
        (uint8_t*)(uintptr_t)
        multiboot_information +
        total_size;

    while (address < end) {
        uint32_t type =
            *(uint32_t*)address;

        uint32_t size =
            *(uint32_t*)(address + 4);

        if (type == 0) {
            break;
        }

        if (type == 6) {
            struct multiboot_memory_map_tag {
                uint32_t type;
                uint32_t size;
                uint32_t entry_size;
                uint32_t entry_version;
            };

            struct multiboot_memory_map_entry {
                uint64_t address;
                uint64_t length;
                uint32_t type;
                uint32_t reserved;
            };

            struct multiboot_memory_map_tag* map =
                (struct multiboot_memory_map_tag*)address;

            uint8_t* entries =
                address + sizeof(
                    struct multiboot_memory_map_tag
                );

            uint8_t* map_end =
                address + map->size;

            while (entries < map_end) {
                struct multiboot_memory_map_entry* entry =
                    (struct multiboot_memory_map_entry*)entries;

                uint64_t start =
                    entry->address;

                uint64_t length =
                    entry->length;

                uint64_t finish =
                    start + length;

                if (entry->type == 1) {
                    if (finish > MAX_MEMORY_BYTES) {
                        finish = MAX_MEMORY_BYTES;
                    }

                    if (start < MAX_MEMORY_BYTES &&
                        finish > start) {

                        uint64_t pages =
                            (finish - start) /
                            PAGE_SIZE;

                        total_pages += pages;

                        free_pages += pages;

                        memory_information.usable_memory +=
                            pages * PAGE_SIZE;

                        reserve_range(
                            0,
                            start
                        );

                        reserve_range(
                            finish,
                            MAX_MEMORY_BYTES
                        );

                        for (
                            uint64_t current =
                                start &
                                ~(uint64_t)(PAGE_SIZE - 1);

                            current < finish;
                            current += PAGE_SIZE
                        ) {
                            uint64_t page =
                                current / PAGE_SIZE;

                            if (page < MAX_PAGES) {
                                bitmap_clear(page);
                            }
                        }
                    }
                } else {
                    if (finish > MAX_MEMORY_BYTES) {
                        finish = MAX_MEMORY_BYTES;
                    }

                    reserve_range(
                        start,
                        finish
                    );

                    memory_information.reserved_memory +=
                        finish > start
                            ? finish - start
                            : 0;
                }

                entries += map->entry_size;
            }
        }

        address +=
            (size + 7) &
            ~((uint32_t)7);
    }

    /*
     * The first 1 MiB is not safe for general allocation.
     */
    reserve_range(0, 0x100000);

    /*
     * The kernel and boot structures live in low
     * physical memory. Reserve the first 16 MiB.
     *
     * This is conservative for now. Once the linker
     * exposes exact kernel bounds we can make this
     * considerably tighter.
     */
    reserve_range(0, 0x1000000);

    memory_information.total_memory =
        memory_information.usable_memory +
        memory_information.reserved_memory;

    if (total_pages == 0) {
        print_str(
            "memory: no usable memory reported\n"
        );
    }
}

const struct memory_info* memory_get_info(void)
{
    return &memory_information;
}

void* page_alloc(void)
{
    for (uint64_t page = 0;
         page < MAX_PAGES;
         page++) {

        if (!bitmap_test(page)) {
            bitmap_set(page);

            if (free_pages > 0) {
                free_pages--;
            }

            return (void*)(uintptr_t)
                (page * PAGE_SIZE);
        }
    }

    return 0;
}

void page_free(void* address)
{
    if (address == 0) {
        return;
    }

    uint64_t physical =
        (uint64_t)(uintptr_t)address;

    if (physical % PAGE_SIZE != 0) {
        return;
    }

    uint64_t page =
        physical / PAGE_SIZE;

    if (page >= MAX_PAGES) {
        return;
    }

    if (bitmap_test(page)) {
        bitmap_clear(page);
        free_pages++;
    }
}

uint64_t page_total(void)
{
    return total_pages;
}

uint64_t page_free_count(void)
{
    return free_pages;
}
