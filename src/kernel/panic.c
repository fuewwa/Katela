#include "../../include/panic.h"
#include "../drivers/console.h"

void panic(const char *message)
{
    clear();
    print("KERNEL PANIC: ");
    print(message);
    print("\nSystem halted.\n");

    asm volatile("cli");

    while (1) {
        asm volatile("hlt");
    }
}
