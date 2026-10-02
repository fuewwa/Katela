#include "../../include/syscall.h"
#include "../../include/panic.h"
#include "../../include/standart.h"
#include "../drivers/vga.h"

static int is_exec(const char *line) {
    return line[0] == 'e' && line[1] == 'x' && line[2] == 'e' && line[3] == 'c' &&
           (line[4] == ' ' || line[4] == '\0');
}

void syscall_dispatch(struct regs *r) {
    switch (r->eax) {
        case SYS_WRITE:
            print((const char *)r->ebx);
            r->eax = 0;
            break;
        case SYS_RUN:
            if (is_exec((const char *)r->ebx)) {
                print("exec: nested exec is not allowed\n");
                r->eax = (unsigned int)-1;
                break;
            }
            execute_command((char *)r->ebx);
            r->eax = 0;
            break;
        default:
            r->eax = (unsigned int)-1;
            break;
    }
}

void gpf_handler(void) {
    panic("general protection fault");
}
