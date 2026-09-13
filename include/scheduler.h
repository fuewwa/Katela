#ifndef SCHEDULER_H
#define SCHEDULER_H

#ifdef __cplusplus
extern "C" {
#endif

void scheduler_init(void);
void scheduler_start(void);

int task_create(void (*entry)(void), const char *name);
void task_yield(void);

unsigned int task_current_id(void);
unsigned int task_count(void);
const char *task_current_name(void);

#ifdef __cplusplus
}
#endif

#endif
