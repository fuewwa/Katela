#include "../../include/syscall.h"
#include "../../include/panic.h"
#include "../../include/standart.h"
#include "../../include/scheduler.h"
#include "../drivers/console.h"
#include "../drivers/keyboard.h"

static int is_exec(const char *line) {
    return line[0] == 'e' && line[1] == 'x' && line[2] == 'e' && line[3] == 'c' &&
           (line[4] == ' ' || line[4] == '\0');
}

unsigned long syscall_dispatch(unsigned long number, unsigned long argument) {
    switch (number) {
        case SYS_WRITE:
            print((const char *)argument);
            return 0;
        case SYS_RUN:
            if (is_exec((const char *)argument)) {
                print("exec: nested exec is not allowed\n");
                return (unsigned long)-1;
            }
            execute_command((char *)argument);
            return 0;
        case SYS_YIELD:
            task_yield();
            return 0;
        case SYS_SLEEP:
            task_sleep(argument);
            return 0;
        case SYS_UPTIME:
            return uptime_ms();
        case SYS_GETKEY:
            return (unsigned long)(unsigned char)get_key();
        default:
            return (unsigned long)-1;
    }
}
