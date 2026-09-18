#include "../../include/syscall.h"
#include "../../include/panic.h"
#include "../../include/standart.h"
#include "../drivers/vga.h"

void syscall_dispatch(struct regs *r) {
    switch (r->eax) {
        case SYS_WRITE:
            print((const char *)r->ebx);
            r->eax = 0;
            break;
        case SYS_RUN:
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
