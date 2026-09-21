#include "../../include/mm.h"
#include "../../include/panic.h"
#include "../../include/mem.h"
#include "../drivers/vga.h"

#define HEAP_SIZE (4 * 1024 * 1024)

struct block {
    unsigned int size;
    int free;
    struct block *next;
};

static unsigned char heap[HEAP_SIZE];
static struct block *first_block = 0;

static void mm_init(void)
{
    first_block = (struct block *)heap;
    first_block->size = HEAP_SIZE - sizeof(struct block);
    first_block->free = 1;
    first_block->next = 0;
}

void *kmalloc(unsigned int size)
{
    struct block *current;
    struct block *new_block;

    if (size == 0)
        return 0;

    if (!first_block)
        mm_init();

    current = first_block;

    while (current) {
        if (current->free && current->size >= size) {
            if (current->size >= size + sizeof(struct block) + 1) {
                new_block = (struct block *)(
                    (unsigned char *)current +
                    sizeof(struct block) +
                    size
                );

                new_block->size =
                    current->size - size - sizeof(struct block);
                new_block->free = 1;
                new_block->next = current->next;

                current->size = size;
                current->next = new_block;
            }

            current->free = 0;

            return (unsigned char *)current + sizeof(struct block);
        }

        current = current->next;
    }

    panic("out of memory");
    return 0;
}

void kfree(void *ptr)
{
    struct block *current;

    if (!ptr)
        return;

    current = first_block;

    while (current) {
        if ((unsigned char *)current + sizeof(struct block) == ptr) {
            current->free = 1;

            if (current->next && current->next->free) {
                current->size +=
                    sizeof(struct block) + current->next->size;

                current->next = current->next->next;
            }

            if (current != first_block) {
                struct block *previous = first_block;

                while (previous->next != current)
                    previous = previous->next;

                if (previous->free) {
                    previous->size +=
                        sizeof(struct block) + current->size;

                    previous->next = current->next;
                }
            }

            return;
        }

        current = current->next;
    }
}

void mm_test()
{
    char *test1 = kmalloc(32);
    char *test2 = kmalloc(64);

    if (!test1 || !test2) {
        panic("kmalloc test failed");
    }

    strcpy(test1, "hello from kmalloc");
    strcpy(test2, "second allocation");

    print(test1);
    print("\n");
    print(test2);
    print("\n");

    kfree(test1);

    char *test3 = kmalloc(16);

    if (!test3) {
        panic("kfree test failed");
    }

    strcpy(test3, "memory reused");
    print(test3);
    print("\n");

    kfree(test2);
    kfree(test3);
}
