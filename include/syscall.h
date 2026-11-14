#ifndef SYSCALL_H
#define SYSCALL_H

#define SYS_WRITE 1
#define SYS_EXIT  2
#define SYS_RUN   3
#define SYS_YIELD  4
#define SYS_SLEEP  5
#define SYS_UPTIME 6
#define SYS_GETKEY 7

unsigned long syscall_dispatch(unsigned long number, unsigned long argument);

#endif
