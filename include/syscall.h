#ifndef SYSCALL_H
#define SYSCALL_H

#define SYS_WRITE 1
#define SYS_EXIT  2
#define SYS_RUN   3
#define SYS_YIELD  4
#define SYS_SLEEP  5
#define SYS_UPTIME 6
#define SYS_GETKEY 7

struct regs {
    unsigned int edi;
    unsigned int esi;
    unsigned int ebp;
    unsigned int esp_dummy;
    unsigned int ebx;
    unsigned int edx;
    unsigned int ecx;
    unsigned int eax;
};

void syscall_dispatch(struct regs *r);
void gpf_handler(void);

#endif
