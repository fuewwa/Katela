#include "../../include/panic.h"
#include "../drivers/console.h"
#include "arch.h"

void panic(const char *message)
{
    clear();
    print("KERNEL PANIC: ");
    print(message);
    print("\nSystem halted.\n");

    cpu_stop();
}
