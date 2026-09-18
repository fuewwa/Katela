#ifndef USERMODE_H
#define USERMODE_H

void enter_usermode(void (*entry)(void), void *user_stack_top);

#endif
