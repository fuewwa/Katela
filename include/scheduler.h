#ifndef SCHEDULER_H
#define SCHEDULER_H

#define TICK_HZ 100

#define WAIT_OK 0
#define WAIT_TIMEOUT (-1)
#define WAIT_FOREVER 0xFFFFFFFFu

#if (1000 % TICK_HZ) != 0
#error "TICK_HZ must divide 1000"
#endif

#ifdef __cplusplus
extern "C" {
#endif

struct task;

struct wait_queue {
    struct task *head;
    struct task *tail;
};

struct semaphore {
    unsigned int count;
    struct wait_queue waiters;
};

struct mutex {
    struct task *owner;
    struct wait_queue waiters;
};

void scheduler_init(void);
void scheduler_start(void);

int task_create(void (*entry)(void), const char *name);
void task_yield(void);
void task_sleep(unsigned int ms);

unsigned int task_current_id(void);
unsigned int task_count(void);
const char *task_current_name(void);

unsigned int timer_ticks(void);
unsigned int uptime_ms(void);

void wait_queue_init(struct wait_queue *q);
int wait_queue_wait_until(struct wait_queue *q, int (*ready)(void *), void *arg, unsigned int timeout_ms);
unsigned int wait_queue_wake_one(struct wait_queue *q);
unsigned int wait_queue_wake_all(struct wait_queue *q);

void sem_init(struct semaphore *s, unsigned int count);
void sem_wait(struct semaphore *s);
int sem_trywait(struct semaphore *s);
int sem_wait_timeout(struct semaphore *s, unsigned int timeout_ms);
void sem_post(struct semaphore *s);

void mutex_init(struct mutex *m);
void mutex_lock(struct mutex *m);
int mutex_trylock(struct mutex *m);
int mutex_lock_timeout(struct mutex *m, unsigned int timeout_ms);
void mutex_unlock(struct mutex *m);

#ifdef __cplusplus
}
#endif

#endif
